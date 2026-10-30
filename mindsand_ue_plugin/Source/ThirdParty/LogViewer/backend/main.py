from __future__ import annotations
import json, os, re, subprocess
from datetime import datetime
from pathlib import Path
from typing import Any
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles

LOG_ROOT = Path(os.environ.get('LOG_ROOT', r'..\..\..\..\..\..\..\Saved\Logs\MindSand')).resolve()
DATE_PATTERN = re.compile(r'^\d{4}-\d{2}-\d{2}$')
FILE_PATTERN = re.compile(r'^(?P<prefix>.+?)_QUERY_[0-9]+_(?P<turn_id>[A-Za-z0-9]+)_(?P<kind>request|response)\.json$', re.I)
FENCE_PATTERN = re.compile(r'^\s*```(?:json)?\s*\n?(.*?)\n?\s*```\s*$', re.S | re.I)
app = FastAPI(title='MindSand Log Viewer')
app.add_middleware(CORSMiddleware, allow_origins=['http://localhost:5173','http://127.0.0.1:5173'], allow_methods=['GET'], allow_headers=['*'])

def read_text(path: Path) -> str:
    return path.read_text(encoding='utf-8', errors='replace')

def file_time(path: Path) -> str:
    return datetime.fromtimestamp(path.stat().st_mtime).isoformat(timespec='seconds')

def scan_turns() -> list[dict[str, Any]]:
    grouped: dict[tuple[str,str], dict[str,Any]] = {}
    if not LOG_ROOT.is_dir(): return []
    for date_dir in LOG_ROOT.iterdir():
        if not date_dir.is_dir() or not DATE_PATTERN.fullmatch(date_dir.name): continue
        for path in date_dir.iterdir():
            match = FILE_PATTERN.fullmatch(path.name) if path.is_file() else None
            if not match: continue
            key = (date_dir.name, match.group('turn_id'))
            item = grouped.setdefault(key, {'date':date_dir.name,'turn_id':match.group('turn_id'),'request':None,'response':None})
            item[match.group('kind').lower()] = {'name':path.name,'path':path,'last_modified':file_time(path)}
    result = []
    for item in grouped.values():
        stamps = [x['last_modified'] for x in (item['request'],item['response']) if x]
        item['latest_modified'] = max(stamps) if stamps else ''
        item['status'] = 'complete' if item['request'] and item['response'] else 'missing request' if item['response'] else 'missing response'
        result.append(item)
    return sorted(result, key=lambda x:(x['date'],x['latest_modified'],x['turn_id']), reverse=True)

def extract_command(raw: str) -> str | None:
    try:
        parsed = json.loads(raw)
        if isinstance(parsed, dict) and parsed.get('command') is not None:
            return str(parsed['command']).strip() or None
    except json.JSONDecodeError:
        pass
    match = re.search(r'^\s*#\s*command\s*\r?\n([^\r\n]+)', raw, re.IGNORECASE | re.MULTILINE)
    return match.group(1).strip() if match else None
def public_turn(item: dict[str,Any]) -> dict[str,Any]:
    command = extract_command(read_text(item['request']['path'])) if item['request'] else None
    indicator = 'red' if item['status'] != 'complete' else 'green' if command else 'orange'
    return {'date':item['date'],'turn_id':item['turn_id'],'status':item['status'],'indicator':indicator,'latest_modified':item['latest_modified'],'command':command,'request_file':item['request']['name'] if item['request'] else None,'response_file':item['response']['name'] if item['response'] else None}

def parse_response(raw: str) -> tuple[Any|None,str|None]:
    candidate = raw.strip(); match = FENCE_PATTERN.match(candidate)
    if match: candidate = match.group(1).strip()
    try: return json.loads(candidate), None
    except json.JSONDecodeError as exc: return None, f'Invalid JSON: {exc.msg} (line {exc.lineno}, column {exc.colno})'

def extract_calls(parsed: Any) -> list[dict[str,Any]]:
    calls = []
    if not isinstance(parsed,dict) or not isinstance(parsed.get('Plans'),list): return calls
    for pi, plan in enumerate(parsed['Plans']):
        if not isinstance(plan,dict) or not isinstance(plan.get('actions'),list): continue
        for ai, action in enumerate(plan['actions']):
            if isinstance(action,dict): calls.append({'plan_index':pi,'action_index':ai,'teammate':plan.get('teammate'),'name':action.get('name'),'parameters':action.get('parameters',{})})
    return calls

@app.get('/api/health')
def health(): return {'status':'ok','log_root':str(LOG_ROOT)}

@app.get('/api/turns')
def turns():
    items = scan_turns()
    return {'root':str(LOG_ROOT),'dates':[{'date':d,'turns':[public_turn(x) for x in items if x['date']==d]} for d in sorted({x['date'] for x in items}, reverse=True)]}

@app.get('/api/turns/{date}/{turn_id}')
def turn_detail(date: str, turn_id: str):
    item = next((x for x in scan_turns() if x['date']==date and x['turn_id']==turn_id), None)
    if item is None: raise HTTPException(404, 'Turn not found')
    request_raw = read_text(item['request']['path']) if item['request'] else None
    response_raw = read_text(item['response']['path']) if item['response'] else None
    parsed, error = parse_response(response_raw) if response_raw is not None else (None,None)
    return {**public_turn(item),'command':extract_command(request_raw) if request_raw else None,'request_text':request_raw,'response_text':response_raw,'parsed_response':parsed,'parse_error':error,'function_calls':extract_calls(parsed)}

@app.get('/api/turns/{date}/{turn_id}/open-location')
def open_turn_location(date: str, turn_id: str):
    item = next((x for x in scan_turns() if x['date'] == date and x['turn_id'] == turn_id), None)
    if item is None: raise HTTPException(404, 'Turn not found')
    target = item['response'] or item['request']
    if target is None: raise HTTPException(404, 'Turn has no files')
    path = target['path'].resolve()
    if os.name == 'nt':
        subprocess.Popen(['explorer.exe', '/select,', str(path)])
    else:
        subprocess.Popen(['xdg-open', str(path.parent)])
    return {'path': str(path)}

frontend_dist = Path(__file__).resolve().parent.parent / 'frontend' / 'dist'
if frontend_dist.is_dir(): app.mount('/', StaticFiles(directory=frontend_dist, html=True), name='frontend')

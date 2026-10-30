# MindSand Log Viewer

## Backend

```powershell
py -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
start_viewer.bat
```

Default log root: `D:\be\Saved\Logs\MindSand`. Override with `$env:LOG_ROOT`.

## Frontend

```powershell
cd frontend
npm install
npm run dev
```

For one-process serving, run `npm run build` first, then `uvicorn backend.main:app --port 8000`.

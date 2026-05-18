// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

"use client";

import { useState, useCallback, useRef, useEffect } from "react";
import {
  BookOpen,
  Zap,
  Database,
  Eye,
  Save,
  Plus,
  Check,
  Copy,
  ChevronDown,
  ChevronRight,
} from "lucide-react";
import { useSessionStore } from "@/store/sessionStore";
import ActionsPanel from "./ActionsPanel";
import PerceptionBuilder from "./PerceptionBuilder";
import JsonEditor from "./JsonEditor";
import { cn } from "@/lib/utils";
import { GameObject, Session } from "@/lib/types";

// ── Prompt Preview Component ──────────────────────────────────────────────────

function PreviewSection({
  title,
  icon,
  children,
  badge,
}: {
  title: string;
  icon: React.ReactNode;
  children: React.ReactNode;
  badge?: string;
}) {
  const [open, setOpen] = useState(false);
  return (
    <div className="border border-gray-700 rounded-lg overflow-hidden">
      <button
        onClick={() => setOpen((v) => !v)}
        className="w-full flex items-center gap-2 px-3 py-2 bg-gray-800/60 hover:bg-gray-800 transition-colors text-left"
      >
        {open ? <ChevronDown className="w-3 h-3 text-gray-500 flex-shrink-0" /> : <ChevronRight className="w-3 h-3 text-gray-500 flex-shrink-0" />}
        <span className="text-gray-400">{icon}</span>
        <span className="text-xs font-semibold text-gray-300 uppercase tracking-wide">{title}</span>
        {badge && <span className="ml-auto text-xs text-gray-500">{badge}</span>}
      </button>
      {open && <div className="px-3 py-2 bg-gray-900/50">{children}</div>}
    </div>
  );
}

function PromptPreview({ session }: { session: Session }) {
  const [copied, setCopied] = useState(false);
  const [globalSystemPrompt, setGlobalSystemPrompt] = useState("");

  useEffect(() => {
    fetch("/api/config")
      .then((r) => r.json())
      .then((d) => setGlobalSystemPrompt(d.systemPrompt ?? ""))
      .catch(() => {});
  }, []);

  const systemText = [
    globalSystemPrompt?.trim(),
    session.scenario?.trim() ? `Scenario: ${session.scenario.trim()}` : "",
  ].filter(Boolean).join("\n");

  let gameStateFormatted = session.gameState;
  try { gameStateFormatted = JSON.stringify(JSON.parse(session.gameState), null, 2); } catch { /* keep */ }

  let perceptionFormatted = session.perceptionState;
  try { perceptionFormatted = JSON.stringify(JSON.parse(session.perceptionState), null, 2); } catch { /* keep */ }

  const actions = session.actions ?? [];

  const handleCopy = async () => {
    const parts = [
      systemText && `=== SYSTEM PROMPT ===\n${systemText}`,
      gameStateFormatted && `=== GAME STATE ===\n${gameStateFormatted}`,
      perceptionFormatted && `=== PERCEPTION ===\n${perceptionFormatted}`,
      actions.length > 0 && `=== ACTIONS ===\n${JSON.stringify(
        actions.map((a) => {
          let params = {};
          try { params = JSON.parse(a.parameters || "{}"); } catch { /* */ }
          return { actionName: a.actionName, parameters: params, description: a.description };
        }),
        null, 2
      )}`,
    ].filter(Boolean).join("\n\n");
    await navigator.clipboard.writeText(parts);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  return (
    <div className="space-y-3">
      <div className="flex items-center justify-between mb-1">
        <p className="text-xs text-gray-500">Live view of what gets sent to the LLM</p>
        <button
          onClick={handleCopy}
          className="flex items-center gap-1 text-xs text-gray-400 hover:text-blue-400 transition-colors"
        >
          <Copy className="w-3 h-3" />
          {copied ? "Copied!" : "Copy all"}
        </button>
      </div>

      {/* System Prompt */}
      <PreviewSection title="System Prompt" icon={<Database className="w-3 h-3" />}>
        {systemText ? (
          <pre className="text-xs text-gray-300 whitespace-pre-wrap leading-relaxed">{systemText}</pre>
        ) : (
          <p className="text-xs text-gray-600 italic">No system prompt set — configure it in the System Prompt tab</p>
        )}
      </PreviewSection>

      {/* Game State */}
      <PreviewSection title="Game State" icon={<Database className="w-3 h-3" />}>
        <pre className="text-xs text-green-300 whitespace-pre-wrap font-mono">{gameStateFormatted || "{}"}</pre>
      </PreviewSection>

      {/* Perception */}
      <PreviewSection title="Perception" icon={<Eye className="w-3 h-3" />}>
        {perceptionFormatted && perceptionFormatted !== "{}" ? (
          <pre className="text-xs text-blue-300 whitespace-pre-wrap font-mono">{perceptionFormatted}</pre>
        ) : (
          <p className="text-xs text-gray-600 italic">No perception state set</p>
        )}
      </PreviewSection>

      {/* Actions */}
      <PreviewSection title="Actions" icon={<Zap className="w-3 h-3" />} badge={`${actions.length}`}>
        {actions.length > 0 ? (
          <div className="space-y-2">
            {actions.map((a) => {
              let params: Record<string, string> = {};
              try { params = JSON.parse(a.parameters || "{}"); } catch { /* */ }
              return (
                <div key={a.id} className="border border-gray-700/50 rounded p-2">
                  <div className="flex items-center gap-2 mb-1">
                    <span className="text-xs font-mono font-bold text-yellow-300">{a.actionName}</span>
                  </div>
                  {a.description && (
                    <p className="text-xs text-gray-500 mb-1">{a.description}</p>
                  )}
                  {Object.keys(params).length > 0 && (
                    <div className="flex flex-wrap gap-1">
                      {Object.entries(params).map(([k, v]) => (
                        <span key={k} className="text-xs bg-gray-800 text-gray-400 rounded px-1.5 py-0.5 font-mono">
                          {k}: <span className="text-yellow-400">{v}</span>
                        </span>
                      ))}
                    </div>
                  )}
                </div>
              );
            })}
          </div>
        ) : (
          <p className="text-xs text-gray-600 italic">No actions defined</p>
        )}
      </PreviewSection>
    </div>
  );
}

type Tab = "scenario" | "actions" | "gameState" | "perception" | "preview";

const TABS: { id: Tab; label: string; icon: React.ReactNode }[] = [
  { id: "scenario", label: "Scenario", icon: <BookOpen className="w-3.5 h-3.5" /> },
  { id: "actions", label: "Actions", icon: <Zap className="w-3.5 h-3.5" /> },
  { id: "gameState", label: "Game State", icon: <Database className="w-3.5 h-3.5" /> },
  { id: "perception", label: "Perception", icon: <Eye className="w-3.5 h-3.5" /> },
  { id: "preview", label: "Preview", icon: <Eye className="w-3.5 h-3.5" /> },
];

function buildPerceptionFromObjects(objects: GameObject[]): string {
  return JSON.stringify(
    {
      TeammatesKnowledge: [
        {
          Teammate: "Player",
          KnownObjects: objects.map((o) => ({
            Id: o.objectId,
            Description: o.description,
          })),
        },
      ],
    },
    null,
    2
  );
}

export default function ConfigPanel() {
  const [activeTab, setActiveTab] = useState<Tab>("scenario");
  const [scenario, setScenario] = useState("");
  const [isDirty, setIsDirty] = useState(false);
  const { activeSession, updateSession } = useSessionStore();
  const saveTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  // Object library for perception selector
  const [availableObjects, setAvailableObjects] = useState<GameObject[]>([]);
  const [selectedObjectIds, setSelectedObjectIds] = useState<Set<string>>(new Set());
  const [objectsLoaded, setObjectsLoaded] = useState(false);

  useEffect(() => {
    if (activeSession) {
      setScenario(activeSession.scenario);
      setIsDirty(false);
    }
  }, [activeSession?.id]);

  // Load available objects when perception tab is opened
  useEffect(() => {
    if (activeTab === "perception" && !objectsLoaded) {
      fetch("/api/objects")
        .then((r) => r.json())
        .then((data) => {
          setAvailableObjects(Array.isArray(data) ? data : []);
          setObjectsLoaded(true);
        })
        .catch(() => setObjectsLoaded(true));
    }
  }, [activeTab, objectsLoaded]);

  const scheduleSave = useCallback(() => {
    if (!activeSession) return;
    if (saveTimerRef.current) clearTimeout(saveTimerRef.current);
    saveTimerRef.current = setTimeout(() => {
      setIsDirty(false);
    }, 2000);
  }, [activeSession]);

  const handleSave = useCallback(async () => {
    if (!activeSession) return;
    await updateSession(activeSession.id, { scenario });
    setIsDirty(false);
    if (saveTimerRef.current) clearTimeout(saveTimerRef.current);
  }, [activeSession, scenario, updateSession]);

  const handleScenarioChange = (value: string) => {
    setScenario(value);
    setIsDirty(true);
    scheduleSave();
  };

  const handleJsonSave = async (field: "gameState" | "perceptionState", value: string) => {
    if (!activeSession) return;
    await updateSession(activeSession.id, { [field]: value });
  };

  const handleToggleObject = (obj: GameObject) => {
    setSelectedObjectIds((prev) => {
      const next = new Set(prev);
      if (next.has(obj.id)) {
        next.delete(obj.id);
      } else {
        next.add(obj.id);
      }
      return next;
    });
  };

  const handleApplyObjects = async () => {
    if (!activeSession) return;
    const selected = availableObjects.filter((o) => selectedObjectIds.has(o.id));
    const perceptionJson = buildPerceptionFromObjects(selected);
    await updateSession(activeSession.id, { perceptionState: perceptionJson });
  };

  if (!activeSession) {
    return (
      <div className="w-[350px] flex-shrink-0 bg-gray-800 border-l border-gray-700 flex items-center justify-center">
        <p className="text-gray-500 text-sm">Select a session to configure</p>
      </div>
    );
  }

  return (
    <div className="w-[350px] flex-shrink-0 bg-gray-800 border-l border-gray-700 flex flex-col">
      {/* Tabs */}
      <div className="border-b border-gray-700 flex-shrink-0">
        <div className="flex overflow-x-auto">
          {TABS.map((tab) => (
            <button
              key={tab.id}
              onClick={() => setActiveTab(tab.id)}
              className={cn(
                "flex items-center gap-1.5 px-3 py-2.5 text-xs font-medium whitespace-nowrap transition-colors border-b-2 -mb-px",
                activeTab === tab.id
                  ? "border-blue-500 text-blue-400"
                  : "border-transparent text-gray-500 hover:text-gray-300"
              )}
            >
              {tab.icon}
              {tab.label}
            </button>
          ))}
        </div>
      </div>

      {/* Tab Content */}
      <div className="flex-1 overflow-y-auto p-4">
        {activeTab === "scenario" && (
          <div className="space-y-3">
            <div className="flex items-center justify-between">
              <label className="text-xs font-medium text-gray-400">
                Scenario
              </label>
              {isDirty && (
                <button
                  onClick={handleSave}
                  className="flex items-center gap-1 text-xs text-blue-400 hover:text-blue-300 transition-colors"
                >
                  <Save className="w-3 h-3" />
                  Save
                </button>
              )}
            </div>
            <textarea
              value={scenario}
              onChange={(e) => handleScenarioChange(e.target.value)}
              onBlur={() => isDirty && handleSave()}
              placeholder="Describe the scenario or context..."
              className="w-full bg-gray-900 border border-gray-600 text-gray-200 text-xs rounded px-3 py-2 focus:outline-none focus:border-blue-500 resize-y min-h-[300px] leading-relaxed"
            />
            <p className="text-xs text-gray-600">
              Provides context about the current situation.
            </p>
          </div>
        )}

        {activeTab === "actions" && (
          <ActionsPanel />
        )}

        {activeTab === "gameState" && (
          <div className="space-y-3">
            <p className="text-xs text-gray-500">
              JSON object representing the current game state.
            </p>
            <JsonEditor
              label="Game State"
              value={activeSession.gameState}
              onChange={(value) => handleJsonSave("gameState", value)}
            />
          </div>
        )}

        {activeTab === "preview" && (
          <PromptPreview session={activeSession} />
        )}

        {activeTab === "perception" && (
          <div className="space-y-4">
            {/* Object selector */}
            {availableObjects.length > 0 && (
              <div className="space-y-2">
                <div className="flex items-center justify-between">
                  <p className="text-xs font-medium text-gray-400">Select Objects</p>
                  <button
                    onClick={handleApplyObjects}
                    disabled={selectedObjectIds.size === 0}
                    className="flex items-center gap-1 text-xs text-blue-400 hover:text-blue-300 disabled:opacity-40 transition-colors"
                  >
                    <Plus className="w-3 h-3" />
                    Apply ({selectedObjectIds.size})
                  </button>
                </div>
                <div className="space-y-1 max-h-48 overflow-y-auto">
                  {availableObjects.map((obj) => (
                    <button
                      key={obj.id}
                      onClick={() => handleToggleObject(obj)}
                      className={cn(
                        "w-full text-left px-2.5 py-2 rounded-lg text-xs transition-colors flex items-center gap-2",
                        selectedObjectIds.has(obj.id)
                          ? "bg-blue-600/20 border border-blue-500/40 text-blue-300"
                          : "bg-gray-700/50 border border-transparent text-gray-400 hover:bg-gray-700"
                      )}
                    >
                      <div className={cn(
                        "w-3.5 h-3.5 rounded border flex items-center justify-center flex-shrink-0",
                        selectedObjectIds.has(obj.id)
                          ? "bg-blue-500 border-blue-500"
                          : "border-gray-500"
                      )}>
                        {selectedObjectIds.has(obj.id) && <Check className="w-2.5 h-2.5 text-white" />}
                      </div>
                      <div className="min-w-0">
                        <span className="font-medium text-gray-200">{obj.name}</span>
                        <span className="ml-1.5 font-mono text-gray-500">{obj.objectId}</span>
                      </div>
                    </button>
                  ))}
                </div>
                <div className="border-t border-gray-700 pt-3" />
              </div>
            )}

            <p className="text-xs text-gray-500">
              What the AI currently perceives — teammates and their known objects.
            </p>
            <PerceptionBuilder
              value={activeSession.perceptionState}
              onChange={(value) => handleJsonSave("perceptionState", value)}
            />
          </div>
        )}
      </div>
    </div>
  );
}
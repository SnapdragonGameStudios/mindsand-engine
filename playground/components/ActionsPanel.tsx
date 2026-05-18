// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

"use client";

import { useState, useEffect } from "react";
import { Plus, Pencil, Trash2, Check, X, ChevronDown, ChevronRight, Library } from "lucide-react";
import { useSessionStore } from "@/store/sessionStore";
import { Action } from "@/lib/types";
import { cn, isValidJson } from "@/lib/utils";

interface ActionFormData {
  actionName: string;
  parameters: string;
  description: string;
}

const defaultForm: ActionFormData = {
  actionName: "",
  parameters: "{}",
  description: "",
};

interface ActionItemProps {
  action: Action;
  onEdit: (action: Action) => void;
  onDelete: (id: string) => void;
}

function ActionItem({ action, onEdit, onDelete }: ActionItemProps) {
  const [expanded, setExpanded] = useState(false);

  return (
    <div className="border border-gray-700 rounded bg-gray-900/50">
      <div className="flex items-center gap-2 px-3 py-2">
        <button
          onClick={() => setExpanded(!expanded)}
          className="text-gray-500 hover:text-gray-300 flex-shrink-0"
        >
          {expanded ? (
            <ChevronDown className="w-3.5 h-3.5" />
          ) : (
            <ChevronRight className="w-3.5 h-3.5" />
          )}
        </button>
        <div className="flex-1 min-w-0">
          <span className="text-xs font-mono text-blue-400 font-medium">
            {action.actionName}
          </span>
          {action.description && (
            <span className="text-xs text-gray-500 ml-2 truncate">
              {action.description}
            </span>
          )}
        </div>
        <div className="flex items-center gap-1 flex-shrink-0">
          <button
            onClick={() => onEdit(action)}
            className="p-1 rounded text-gray-500 hover:text-gray-300 transition-colors"
          >
            <Pencil className="w-3 h-3" />
          </button>
          <button
            onClick={() => onDelete(action.id)}
            className="p-1 rounded text-gray-500 hover:text-red-400 transition-colors"
          >
            <Trash2 className="w-3 h-3" />
          </button>
        </div>
      </div>
      {expanded && (
        <div className="px-3 pb-2 border-t border-gray-700/50 pt-2">
          <p className="text-xs text-gray-500 mb-1">Parameters:</p>
          <pre className="text-xs font-mono text-gray-400 bg-gray-900 rounded p-2 overflow-x-auto">
            {action.parameters}
          </pre>
        </div>
      )}
    </div>
  );
}

interface ActionEditorProps {
  initial?: ActionFormData;
  onSave: (data: ActionFormData) => void;
  onCancel: () => void;
}

function ActionEditor({ initial = defaultForm, onSave, onCancel }: ActionEditorProps) {
  const [form, setForm] = useState<ActionFormData>(initial);
  const [paramsError, setParamsError] = useState(false);
  const [library, setLibrary] = useState<Action[]>([]);
  const [showLibrary, setShowLibrary] = useState(false);

  useEffect(() => {
    fetch("/api/actions")
      .then((r) => r.json())
      .then((data) => setLibrary(Array.isArray(data) ? data : []))
      .catch(() => {});
  }, []);

  const handleParamsChange = (value: string) => {
    setForm({ ...form, parameters: value });
    setParamsError(!isValidJson(value));
  };

  const handleSubmit = () => {
    if (!form.actionName.trim()) return;
    if (!isValidJson(form.parameters)) return;
    onSave(form);
  };

  return (
    <div className="border border-blue-500/30 rounded bg-gray-900/80 p-3 space-y-2">
      {library.length > 0 && (
        <div>
          <button
            onClick={() => setShowLibrary(!showLibrary)}
            className="flex items-center gap-1.5 text-xs text-gray-400 hover:text-gray-200 transition-colors"
          >
            <Library className="w-3 h-3" />
            Pick from previous actions
            {showLibrary ? <ChevronDown className="w-3 h-3" /> : <ChevronRight className="w-3 h-3" />}
          </button>
          {showLibrary && (
            <div className="mt-1.5 border border-gray-700 rounded bg-gray-900 divide-y divide-gray-700/50 max-h-36 overflow-y-auto">
              {library.map((a) => (
                <button
                  key={a.id}
                  onClick={() => { setForm({ actionName: a.actionName, parameters: a.parameters, description: a.description }); setShowLibrary(false); }}
                  className="w-full text-left px-3 py-2 hover:bg-gray-800 transition-colors"
                >
                  <span className="text-xs font-mono text-blue-400">{a.actionName}</span>
                  {a.description && <span className="text-xs text-gray-500 ml-2">{a.description}</span>}
                </button>
              ))}
            </div>
          )}
        </div>
      )}
      <div>
        <label className="text-xs text-gray-400 block mb-1">Action Name *</label>
        <input
          type="text"
          value={form.actionName}
          onChange={(e) => setForm({ ...form, actionName: e.target.value })}
          placeholder="e.g., move_player"
          className="w-full bg-gray-800 border border-gray-600 text-gray-200 text-xs rounded px-2 py-1.5 focus:outline-none focus:border-blue-500"
        />
      </div>
      <div>
        <label className="text-xs text-gray-400 block mb-1">Description</label>
        <input
          type="text"
          value={form.description}
          onChange={(e) => setForm({ ...form, description: e.target.value })}
          placeholder="What does this action do?"
          className="w-full bg-gray-800 border border-gray-600 text-gray-200 text-xs rounded px-2 py-1.5 focus:outline-none focus:border-blue-500"
        />
      </div>
      <div>
        <label className="text-xs text-gray-400 block mb-1">Parameters (JSON)</label>
        <textarea
          value={form.parameters}
          onChange={(e) => handleParamsChange(e.target.value)}
          rows={3}
          spellCheck={false}
          className={cn(
            "w-full font-mono bg-gray-800 text-gray-200 text-xs rounded px-2 py-1.5 focus:outline-none resize-none",
            paramsError
              ? "border border-red-500/50 focus:border-red-500"
              : "border border-gray-600 focus:border-blue-500"
          )}
        />
        {paramsError && (
          <p className="text-xs text-red-400 mt-0.5">Invalid JSON</p>
        )}
      </div>
      <div className="flex items-center gap-2 justify-end">
        <button
          onClick={onCancel}
          className="flex items-center gap-1 px-2.5 py-1.5 text-xs text-gray-400 hover:text-gray-200 rounded border border-gray-600 hover:border-gray-500 transition-colors"
        >
          <X className="w-3 h-3" />
          Cancel
        </button>
        <button
          onClick={handleSubmit}
          disabled={!form.actionName.trim() || paramsError}
          className="flex items-center gap-1 px-2.5 py-1.5 text-xs bg-blue-600 hover:bg-blue-500 disabled:bg-gray-700 disabled:text-gray-500 text-white rounded transition-colors"
        >
          <Check className="w-3 h-3" />
          Save
        </button>
      </div>
    </div>
  );
}

export default function ActionsPanel() {
  const { activeSession, addAction, updateAction, deleteAction } =
    useSessionStore();
  const [showAddForm, setShowAddForm] = useState(false);
  const [editingId, setEditingId] = useState<string | null>(null);

  if (!activeSession) {
    return (
      <div className="text-center py-6 text-gray-500 text-xs">
        No session selected
      </div>
    );
  }

  const actions = activeSession.actions || [];

  const handleAdd = async (data: ActionFormData) => {
    await addAction(data);
    setShowAddForm(false);
  };

  const handleEdit = (action: Action) => {
    setEditingId(action.id);
    setShowAddForm(false);
  };

  const handleUpdate = async (data: ActionFormData) => {
    if (editingId) {
      await updateAction(editingId, data);
      setEditingId(null);
    }
  };

  const handleDelete = async (id: string) => {
    if (confirm("Delete this action?")) {
      await deleteAction(id);
    }
  };

  return (
    <div className="space-y-2">
      <div className="flex items-center justify-between">
        <span className="text-xs text-gray-400">
          {actions.length} action{actions.length !== 1 ? "s" : ""}
        </span>
        {!showAddForm && (
          <button
            onClick={() => {
              setShowAddForm(true);
              setEditingId(null);
            }}
            className="flex items-center gap-1 px-2 py-1 text-xs text-blue-400 hover:text-blue-300 rounded border border-blue-500/30 hover:border-blue-400/50 transition-colors"
          >
            <Plus className="w-3 h-3" />
            Add Action
          </button>
        )}
      </div>

      {showAddForm && (
        <ActionEditor
          onSave={handleAdd}
          onCancel={() => setShowAddForm(false)}
        />
      )}

      <div className="space-y-1.5">
        {actions.map((action) =>
          editingId === action.id ? (
            <ActionEditor
              key={action.id}
              initial={{
                actionName: action.actionName,
                parameters: action.parameters,
                description: action.description,
              }}
              onSave={handleUpdate}
              onCancel={() => setEditingId(null)}
            />
          ) : (
            <ActionItem
              key={action.id}
              action={action}
              onEdit={handleEdit}
              onDelete={handleDelete}
            />
          )
        )}
      </div>

      {actions.length === 0 && !showAddForm && (
        <div className="text-center py-4 text-gray-600 text-xs">
          No actions defined
        </div>
      )}
    </div>
  );
}

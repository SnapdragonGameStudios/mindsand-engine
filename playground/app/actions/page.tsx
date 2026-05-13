// Copyright Qualcomm Innovation Center, Inc. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

"use client";

import { useEffect, useState } from "react";
import { Zap, Loader2, Plus, X, Pencil, Trash2 } from "lucide-react";
import Header from "@/components/Header";
import { LibraryAction, LibraryActionParam } from "@/lib/types";

const PARAM_TYPES = ["Float", "String", "Actor"] as const;

interface ActionForm {
  name: string;
  description: string;
  parameters: { name: string; type: "Float" | "String" | "Actor" }[];
}

const DEFAULT_FORM: ActionForm = { name: "", description: "", parameters: [] };

export default function ActionsPage() {
  const [actions, setActions] = useState<LibraryAction[]>([]);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [showModal, setShowModal] = useState(false);
  const [editingAction, setEditingAction] = useState<LibraryAction | null>(null);
  const [form, setForm] = useState<ActionForm>(DEFAULT_FORM);
  const [isSaving, setIsSaving] = useState(false);
  const [formError, setFormError] = useState<string | null>(null);

  useEffect(() => {
    fetchActions();
  }, []);

  const fetchActions = async () => {
    try {
      const res = await fetch("/api/action-library");
      if (!res.ok) throw new Error("Failed to fetch actions");
      setActions(await res.json());
    } catch (err) {
      setError(err instanceof Error ? err.message : "Unknown error");
    } finally {
      setIsLoading(false);
    }
  };

  const openCreate = () => {
    setEditingAction(null);
    setForm(DEFAULT_FORM);
    setFormError(null);
    setShowModal(true);
  };

  const openEdit = (action: LibraryAction) => {
    setEditingAction(action);
    setForm({ name: action.name, description: action.description, parameters: action.parameters.map((p) => ({ ...p })) });
    setFormError(null);
    setShowModal(true);
  };

  const closeModal = () => {
    setShowModal(false);
    setEditingAction(null);
    setForm(DEFAULT_FORM);
    setFormError(null);
  };

  const addParam = () => {
    setForm((f) => ({ ...f, parameters: [...f.parameters, { name: "", type: "Float" }] }));
  };

  const removeParam = (idx: number) => {
    setForm((f) => ({ ...f, parameters: f.parameters.filter((_, i) => i !== idx) }));
  };

  const updateParam = (idx: number, field: keyof LibraryActionParam, val: string) => {
    setForm((f) => ({
      ...f,
      parameters: f.parameters.map((p, i) => i === idx ? { ...p, [field]: val } : p),
    }));
  };

  const handleSave = async () => {
    if (!form.name.trim()) { setFormError("Name is required"); return; }
    setIsSaving(true);
    setFormError(null);
    try {
      const payload = {
        name: form.name.trim(),
        description: form.description.trim(),
        parameters: form.parameters,
      };
      if (editingAction) {
        const res = await fetch(`/api/action-library/${editingAction.id}`, {
          method: "PUT",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(payload),
        });
        if (!res.ok) throw new Error((await res.json()).error || "Failed to update");
        const updated = await res.json();
        if (updated.id !== editingAction.id) {
          setActions((prev) => prev.map((a) => a.id === editingAction.id ? updated : a));
        } else {
          setActions((prev) => prev.map((a) => (a.id === updated.id ? updated : a)));
        }
      } else {
        const res = await fetch("/api/action-library", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(payload),
        });
        if (!res.ok) throw new Error((await res.json()).error || "Failed to create");
        const created = await res.json();
        setActions((prev) => [created, ...prev]);
      }
      closeModal();
    } catch (err) {
      setFormError(err instanceof Error ? err.message : "Unknown error");
    } finally {
      setIsSaving(false);
    }
  };

  const handleDelete = async (action: LibraryAction) => {
    if (!confirm(`Delete "${action.name}"?`)) return;
    await fetch(`/api/action-library/${action.id}`, { method: "DELETE" });
    setActions((prev) => prev.filter((a) => a.id !== action.id));
  };

  return (
    <div className="h-screen flex flex-col overflow-hidden bg-gray-900">
      <Header />
      <div className="flex-1 overflow-y-auto">
        <div className="max-w-6xl mx-auto px-6 py-8">
          <div className="mb-8 flex items-start justify-between">
            <div>
              <div className="flex items-center gap-3 mb-2">
                <Zap className="w-6 h-6 text-blue-400" />
                <h1 className="text-2xl font-semibold text-white">Action Library</h1>
              </div>
              <p className="text-gray-400 text-sm">
                Define reusable actions with typed parameters for use in scenarios.
              </p>
            </div>
            <button onClick={openCreate} className="flex items-center gap-2 px-4 py-2 bg-blue-600 hover:bg-blue-500 text-white text-sm rounded-lg transition-colors">
              <Plus className="w-4 h-4" />
              New Action
            </button>
          </div>

          {isLoading && <div className="flex items-center justify-center py-20"><Loader2 className="w-8 h-8 text-gray-500 animate-spin" /></div>}
          {error && <div className="bg-red-900/20 border border-red-700/30 rounded-xl p-6 text-center"><p className="text-red-300">{error}</p></div>}

          {!isLoading && !error && actions.length === 0 && (
            <div className="flex flex-col items-center justify-center py-20 text-center">
              <Zap className="w-16 h-16 text-gray-700 mb-4" />
              <h2 className="text-gray-400 text-lg font-medium mb-2">No Actions Yet</h2>
              <p className="text-gray-600 text-sm">Click <span className="text-gray-400">New Action</span> to create one.</p>
            </div>
          )}

          {!isLoading && actions.length > 0 && (
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
              {actions.map((action) => (
                <div key={action.id} className="bg-gray-800 border border-gray-700 rounded-xl p-5 group hover:border-blue-500/40 transition-all">
                  <div className="flex items-start justify-between gap-2 mb-2">
                    <h3 className="font-mono font-medium text-blue-300 text-sm truncate">{action.name}</h3>
                  </div>
                  {action.parameters.length > 0 && (
                    <div className="flex flex-wrap gap-1 mb-2">
                      {action.parameters.map((p, i) => (
                        <span key={i} className="text-xs bg-gray-700 text-gray-300 rounded px-1.5 py-0.5 font-mono">
                          {p.name}: <span className="text-yellow-400">{p.type}</span>
                        </span>
                      ))}
                    </div>
                  )}
                  <p className="text-xs text-gray-500 mb-4 line-clamp-2">{action.description || "No description"}</p>
                  <div className="flex items-center justify-end gap-1 opacity-0 group-hover:opacity-100 transition-opacity">
                    <button onClick={() => openEdit(action)} className="flex items-center gap-1 text-xs text-gray-400 hover:text-blue-400 transition-colors px-2 py-1 rounded hover:bg-blue-400/10">
                      <Pencil className="w-3 h-3" /> Edit
                    </button>
                    <button onClick={() => handleDelete(action)} className="flex items-center gap-1 text-xs text-gray-400 hover:text-red-400 transition-colors px-2 py-1 rounded hover:bg-red-400/10">
                      <Trash2 className="w-3 h-3" /> Delete
                    </button>
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>
      </div>

      {showModal && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/60">
          <div className="bg-gray-800 rounded-xl shadow-xl w-full max-w-lg mx-4 flex flex-col max-h-[90vh]">
            <div className="flex items-center justify-between px-6 py-4 border-b border-gray-700">
              <h2 className="text-white font-semibold">{editingAction ? "Edit Action" : "New Action"}</h2>
              <button onClick={closeModal} className="text-gray-400 hover:text-white transition-colors"><X className="w-5 h-5" /></button>
            </div>

            <div className="flex-1 overflow-y-auto px-6 py-4 space-y-4">
              {formError && <div className="bg-red-900/20 border border-red-700/30 rounded-lg px-4 py-2 text-red-300 text-sm">{formError}</div>}

              <div>
                <label className="block text-xs text-gray-400 mb-1">Name *</label>
                <input type="text" value={form.name} onChange={(e) => setForm({ ...form, name: e.target.value })}
                  placeholder="e.g. moveForward" className="w-full bg-gray-900 border border-gray-700 rounded-lg px-3 py-2 text-white text-sm font-mono placeholder-gray-500 focus:outline-none focus:border-blue-500" />
              </div>

              <div>
                <label className="block text-xs text-gray-400 mb-1">Description</label>
                <textarea value={form.description} onChange={(e) => setForm({ ...form, description: e.target.value })}
                  placeholder="What this action does..." rows={2}
                  className="w-full bg-gray-900 border border-gray-700 rounded-lg px-3 py-2 text-white text-sm placeholder-gray-500 focus:outline-none focus:border-blue-500 resize-none" />
              </div>

              <div>
                <div className="flex items-center justify-between mb-2">
                  <label className="block text-xs text-gray-400">Parameters</label>
                  <button type="button" onClick={addParam}
                    className="flex items-center gap-1 text-xs text-blue-400 hover:text-blue-300 transition-colors">
                    <Plus className="w-3 h-3" /> Add
                  </button>
                </div>
                {form.parameters.length === 0 && (
                  <p className="text-xs text-gray-600 italic">No parameters.</p>
                )}
                <div className="space-y-2">
                  {form.parameters.map((p, i) => (
                    <div key={i} className="flex items-center gap-2">
                      <input type="text" value={p.name} onChange={(e) => updateParam(i, "name", e.target.value)}
                        placeholder="param_name"
                        className="flex-1 bg-gray-900 border border-gray-700 rounded-lg px-3 py-1.5 text-white text-xs font-mono placeholder-gray-600 focus:outline-none focus:border-blue-500" />
                      <select value={p.type} onChange={(e) => updateParam(i, "type", e.target.value)}
                        className="bg-gray-900 border border-gray-700 text-yellow-400 text-xs rounded-lg px-2 py-1.5 focus:outline-none focus:border-blue-500">
                        {PARAM_TYPES.map((t) => <option key={t} value={t}>{t}</option>)}
                      </select>
                      <button type="button" onClick={() => removeParam(i)} className="p-1 text-gray-600 hover:text-red-400 transition-colors">
                        <X className="w-3.5 h-3.5" />
                      </button>
                    </div>
                  ))}
                </div>
              </div>
            </div>

            <div className="flex justify-end gap-3 px-6 py-4 border-t border-gray-700">
              <button onClick={closeModal} className="px-4 py-2 text-sm text-gray-400 hover:text-white transition-colors">Cancel</button>
              <button onClick={handleSave} disabled={isSaving} className="flex items-center gap-2 px-4 py-2 bg-blue-600 hover:bg-blue-500 disabled:opacity-50 text-white text-sm rounded-lg transition-colors">
                {isSaving && <Loader2 className="w-4 h-4 animate-spin" />}
                {editingAction ? "Save Changes" : "Create"}
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
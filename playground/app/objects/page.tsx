// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

"use client";

import { useEffect, useState } from "react";
import { Package, Loader2, Plus, X, Pencil, Trash2 } from "lucide-react";
import Header from "@/components/Header";
import { GameObject } from "@/lib/types";

interface ObjectForm {
  objectId: string;
  name: string;
  description: string;
}

function nextObjectId(objects: { objectId: string }[]) {
  const nums = objects
    .map((o) => { const m = o.objectId.match(/^obj_(\d+)$/); return m ? parseInt(m[1], 10) : 0; })
    .filter((n) => n > 0);
  const next = nums.length > 0 ? Math.max(...nums) + 1 : 1;
  return `obj_${String(next).padStart(3, "0")}`;
}

const DEFAULT_FORM: ObjectForm = { objectId: "", name: "", description: "" };

export default function ObjectsPage() {
  const [objects, setObjects] = useState<GameObject[]>([]);
  const [isLoading, setIsLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [showModal, setShowModal] = useState(false);
  const [editingObject, setEditingObject] = useState<GameObject | null>(null);
  const [form, setForm] = useState<ObjectForm>(DEFAULT_FORM);
  const [isSaving, setIsSaving] = useState(false);
  const [formError, setFormError] = useState<string | null>(null);

  useEffect(() => { fetchObjects(); }, []);

  const fetchObjects = async () => {
    try {
      const res = await fetch("/api/objects");
      if (!res.ok) throw new Error("Failed to fetch objects");
      setObjects(await res.json());
    } catch (err) {
      setError(err instanceof Error ? err.message : "Unknown error");
    } finally {
      setIsLoading(false);
    }
  };

  const openCreate = () => {
    setEditingObject(null);
    setForm({ objectId: nextObjectId(objects), name: "", description: "" });
    setFormError(null);
    setShowModal(true);
  };

  const openEdit = (obj: GameObject) => {
    setEditingObject(obj);
    setForm({
      objectId: obj.objectId,
      name: obj.name,
      description: obj.description,
    });
    setFormError(null);
    setShowModal(true);
  };

  const closeModal = () => {
    setShowModal(false);
    setEditingObject(null);
    setForm(DEFAULT_FORM);
    setFormError(null);
  };

  const handleSave = async () => {
    if (!form.objectId.trim()) { setFormError("Object ID is required"); return; }
    if (!form.name.trim()) { setFormError("Name is required"); return; }
    setIsSaving(true);
    setFormError(null);
    try {
      const payload = {
        objectId: form.objectId.trim(),
        name: form.name.trim(),
        description: form.description.trim(),
        location: { x: 0, y: 0, z: 0 },
      };
      if (editingObject) {
        const res = await fetch(`/api/objects/${editingObject.id}`, {
          method: "PUT",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(payload),
        });
        if (!res.ok) throw new Error((await res.json()).error || "Failed to update");
        const updated = await res.json();
        setObjects((prev) => prev.map((o) => (o.id === updated.id ? updated : o)));
      } else {
        const res = await fetch("/api/objects", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(payload),
        });
        const created = await res.json();
        if (!res.ok) throw new Error(created.error || "Failed to create");
        setObjects((prev) => [created, ...prev]);
      }
      closeModal();
    } catch (err) {
      setFormError(err instanceof Error ? err.message : "Unknown error");
    } finally {
      setIsSaving(false);
    }
  };

  const handleDelete = async (obj: GameObject) => {
    if (!confirm(`Delete "${obj.name}"?`)) return;
    await fetch(`/api/objects/${obj.id}`, { method: "DELETE" });
    setObjects((prev) => prev.filter((o) => o.id !== obj.id));
  };

  return (
    <div className="h-screen flex flex-col overflow-hidden bg-gray-900">
      <Header />
      <div className="flex-1 overflow-y-auto">
        <div className="max-w-6xl mx-auto px-6 py-8">
          <div className="mb-8 flex items-start justify-between">
            <div>
              <div className="flex items-center gap-3 mb-2">
                <Package className="w-6 h-6 text-blue-400" />
                <h1 className="text-2xl font-semibold text-white">Object Library</h1>
              </div>
              <p className="text-gray-400 text-sm">
                Define objects for use in scenario perception states.
              </p>
            </div>
            <button onClick={openCreate} className="flex items-center gap-2 px-4 py-2 bg-blue-600 hover:bg-blue-500 text-white text-sm rounded-lg transition-colors">
              <Plus className="w-4 h-4" />
              New Object
            </button>
          </div>

          {isLoading && <div className="flex items-center justify-center py-20"><Loader2 className="w-8 h-8 text-gray-500 animate-spin" /></div>}
          {error && <div className="bg-red-900/20 border border-red-700/30 rounded-xl p-6 text-center"><p className="text-red-300">{error}</p></div>}

          {!isLoading && !error && objects.length === 0 && (
            <div className="flex flex-col items-center justify-center py-20 text-center">
              <Package className="w-16 h-16 text-gray-700 mb-4" />
              <h2 className="text-gray-400 text-lg font-medium mb-2">No Objects Yet</h2>
              <p className="text-gray-600 text-sm">Click <span className="text-gray-400">New Object</span> to create one.</p>
            </div>
          )}

          {!isLoading && objects.length > 0 && (
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
              {objects.map((obj) => (
                <div key={obj.id} className="bg-gray-800 border border-gray-700 rounded-xl p-5 group hover:border-blue-500/40 transition-all">
                  <div className="flex items-start justify-between gap-2 mb-1">
                    <h3 className="font-medium text-white text-sm truncate">{obj.name}</h3>
                    <span className="flex-shrink-0 text-xs bg-blue-500/15 text-blue-300 border border-blue-500/25 rounded-full px-2 py-0.5 font-mono">
                      {obj.objectId}
                    </span>
                  </div>
                  <p className="text-xs text-gray-500 mb-4 line-clamp-2">{obj.description || "No description"}</p>
                  <div className="flex items-center justify-end gap-1 opacity-0 group-hover:opacity-100 transition-opacity">
                    <button onClick={() => openEdit(obj)} className="flex items-center gap-1 text-xs text-gray-400 hover:text-blue-400 transition-colors px-2 py-1 rounded hover:bg-blue-400/10">
                      <Pencil className="w-3 h-3" /> Edit
                    </button>
                    <button onClick={() => handleDelete(obj)} className="flex items-center gap-1 text-xs text-gray-400 hover:text-red-400 transition-colors px-2 py-1 rounded hover:bg-red-400/10">
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
          <div className="bg-gray-800 rounded-xl shadow-xl w-full max-w-lg mx-4 flex flex-col">
            <div className="flex items-center justify-between px-6 py-4 border-b border-gray-700">
              <h2 className="text-white font-semibold">{editingObject ? "Edit Object" : "New Object"}</h2>
              <button onClick={closeModal} className="text-gray-400 hover:text-white transition-colors"><X className="w-5 h-5" /></button>
            </div>

            <div className="px-6 py-4 space-y-4">
              {formError && <div className="bg-red-900/20 border border-red-700/30 rounded-lg px-4 py-2 text-red-300 text-sm">{formError}</div>}

              <div>
                <label className="block text-xs text-gray-400 mb-1">Object ID * <span className="text-gray-600">(unique identifier sent in perception JSON)</span></label>
                <input type="text" value={form.objectId} onChange={(e) => setForm({ ...form, objectId: e.target.value })}
                  placeholder="e.g. obj_001" className="w-full bg-gray-900 border border-gray-700 rounded-lg px-3 py-2 text-white text-sm font-mono placeholder-gray-500 focus:outline-none focus:border-blue-500" />
              </div>

              <div>
                <label className="block text-xs text-gray-400 mb-1">Name * <span className="text-gray-600">(internal label, not sent to LLM)</span></label>
                <input type="text" value={form.name} onChange={(e) => setForm({ ...form, name: e.target.value })}
                  placeholder="e.g. Red Crate" className="w-full bg-gray-900 border border-gray-700 rounded-lg px-3 py-2 text-white text-sm placeholder-gray-500 focus:outline-none focus:border-blue-500" />
              </div>

              <div>
                <label className="block text-xs text-gray-400 mb-1">Description</label>
                <textarea value={form.description} onChange={(e) => setForm({ ...form, description: e.target.value })}
                  placeholder="Describe the object..." rows={2}
                  className="w-full bg-gray-900 border border-gray-700 rounded-lg px-3 py-2 text-white text-sm placeholder-gray-500 focus:outline-none focus:border-blue-500 resize-none" />
              </div>
            </div>

            <div className="flex justify-end gap-3 px-6 py-4 border-t border-gray-700">
              <button onClick={closeModal} className="px-4 py-2 text-sm text-gray-400 hover:text-white transition-colors">Cancel</button>
              <button onClick={handleSave} disabled={isSaving} className="flex items-center gap-2 px-4 py-2 bg-blue-600 hover:bg-blue-500 disabled:opacity-50 text-white text-sm rounded-lg transition-colors">
                {isSaving && <Loader2 className="w-4 h-4 animate-spin" />}
                {editingObject ? "Save Changes" : "Create"}
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

"use client";

import { useState, useEffect, useCallback } from "react";
import { Plus, X, UserPlus } from "lucide-react";
import { GameObject } from "@/lib/types";

interface TeammateEntry {
  id: string;
  objectIds: string[]; // references GameObject.id (internal)
}

interface PerceptionBuilderProps {
  value: string;
  onChange: (json: string) => void;
}

function buildPerceptionJson(teammates: TeammateEntry[], objects: GameObject[]): string {
  return JSON.stringify(
    {
      TeammatesKnowledge: teammates.map((t) => ({
        Teammate: t.id,
        KnownObjects: t.objectIds
          .map((oid) => objects.find((o) => o.id === oid))
          .filter(Boolean)
          .map((o) => ({
            Id: o!.objectId,
            Location: { x: o!.location.x, y: o!.location.y, z: o!.location.z ?? 0 },
            Description: o!.description,
          })),
      })),
    },
    null,
    2
  );
}

function parseTeammatesFromJson(jsonStr: string, objects: GameObject[]): TeammateEntry[] {
  try {
    const parsed = JSON.parse(jsonStr);
    if (!parsed?.TeammatesKnowledge || !Array.isArray(parsed.TeammatesKnowledge)) return [];
    return parsed.TeammatesKnowledge.map(
      (tk: { Teammate: string; KnownObjects?: { Id: string }[] }) => {
        const objectIds = (tk.KnownObjects ?? [])
          .map((ko) => objects.find((o) => o.objectId === ko.Id)?.id)
          .filter((id): id is string => Boolean(id));
        return { id: tk.Teammate ?? "", objectIds };
      }
    );
  } catch {
    return [];
  }
}

export default function PerceptionBuilder({ value, onChange }: PerceptionBuilderProps) {
  const [availableObjects, setAvailableObjects] = useState<GameObject[]>([]);
  const [teammates, setTeammates] = useState<TeammateEntry[]>([]);
  const [initialized, setInitialized] = useState(false);

  useEffect(() => {
    fetch("/api/objects")
      .then((r) => r.json())
      .then((data: GameObject[]) => {
        setAvailableObjects(data);
        if (!initialized) {
          setTeammates(parseTeammatesFromJson(value, data));
          setInitialized(true);
        }
      })
      .catch(() => { setInitialized(true); });
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const emit = useCallback(
    (newTeammates: TeammateEntry[]) => {
      onChange(buildPerceptionJson(newTeammates, availableObjects));
    },
    [availableObjects, onChange]
  );

  const addTeammate = () => {
    const updated = [...teammates, { id: "", objectIds: [] }];
    setTeammates(updated);
    emit(updated);
  };

  const removeTeammate = (idx: number) => {
    const updated = teammates.filter((_, i) => i !== idx);
    setTeammates(updated);
    emit(updated);
  };

  const updateTeammateId = (idx: number, id: string) => {
    const updated = teammates.map((t, i) => (i === idx ? { ...t, id } : t));
    setTeammates(updated);
    emit(updated);
  };

  const toggleObject = (teammateIdx: number, objectId: string) => {
    const updated = teammates.map((t, i) => {
      if (i !== teammateIdx) return t;
      const has = t.objectIds.includes(objectId);
      return { ...t, objectIds: has ? t.objectIds.filter((oid) => oid !== objectId) : [...t.objectIds, objectId] };
    });
    setTeammates(updated);
    emit(updated);
  };

  return (
    <div className="space-y-3">
      <button type="button" onClick={addTeammate}
        className="flex items-center gap-1.5 text-xs text-blue-400 hover:text-blue-300 transition-colors px-2 py-1 bg-blue-500/10 border border-blue-500/20 rounded">
        <UserPlus className="w-3.5 h-3.5" />
        Add Teammate
      </button>

      {teammates.length === 0 && (
        <p className="text-xs text-gray-600 italic">No teammates configured.</p>
      )}

      {teammates.map((teammate, idx) => (
        <div key={idx} className="bg-gray-900 border border-gray-700 rounded-lg p-3 space-y-2">
          <div className="flex items-center gap-2">
            <input type="text" value={teammate.id} onChange={(e) => updateTeammateId(idx, e.target.value)}
              placeholder="teammate_id"
              className="flex-1 bg-gray-800 border border-gray-600 text-gray-200 text-xs rounded px-2 py-1 focus:outline-none focus:border-blue-500 font-mono" />
            <button type="button" onClick={() => removeTeammate(idx)} className="p-1 text-gray-500 hover:text-red-400 transition-colors">
              <X className="w-3.5 h-3.5" />
            </button>
          </div>

          {availableObjects.length === 0 ? (
            <p className="text-xs text-gray-600 italic pl-1">No objects in library yet.</p>
          ) : (
            <div className="space-y-1 pl-1">
              <p className="text-xs text-gray-500 font-medium mb-1">Known Objects:</p>
              {availableObjects.map((obj) => {
                const checked = teammate.objectIds.includes(obj.id);
                return (
                  <label key={obj.id} className="flex items-center gap-2 cursor-pointer group">
                    <input type="checkbox" checked={checked} onChange={() => toggleObject(idx, obj.id)}
                      className="w-3.5 h-3.5 rounded border-gray-600 bg-gray-800 text-blue-500 focus:ring-0 focus:ring-offset-0" />
                    <span className={`text-xs ${checked ? "text-gray-200" : "text-gray-500"} group-hover:text-gray-300 transition-colors`}>
                      {obj.name}
                      <span className="text-gray-600 ml-1 font-mono">({obj.objectId})</span>
                      <span className="text-gray-700 ml-1">x:{obj.location.x} y:{obj.location.y} z:{obj.location.z ?? 0}</span>
                    </span>
                  </label>
                );
              })}
            </div>
          )}
        </div>
      ))}
    </div>
  );
}

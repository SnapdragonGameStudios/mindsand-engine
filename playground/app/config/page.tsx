// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

"use client";

import { useEffect, useState, useRef } from "react";
import { Settings, Save } from "lucide-react";
import Header from "@/components/Header";

export default function ConfigPage() {
  const [systemPrompt, setSystemPrompt] = useState("");
  const [isDirty, setIsDirty] = useState(false);
  const saveTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  useEffect(() => {
    fetch("/api/config")
      .then((r) => r.json())
      .then((data) => { if (typeof data.systemPrompt === "string") setSystemPrompt(data.systemPrompt); })
      .catch(() => {});
  }, []);

  const handleChange = (value: string) => {
    setSystemPrompt(value);
    setIsDirty(true);
    if (saveTimerRef.current) clearTimeout(saveTimerRef.current);
    saveTimerRef.current = setTimeout(() => setIsDirty(false), 2000);
  };

  const save = async () => {
    await fetch("/api/config", {
      method: "PUT",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ systemPrompt }),
    });
    setIsDirty(false);
    if (saveTimerRef.current) clearTimeout(saveTimerRef.current);
  };

  return (
    <div className="h-screen flex flex-col overflow-hidden bg-gray-900">
      <Header />
      <div className="flex-1 overflow-y-auto">
        <div className="max-w-3xl mx-auto px-6 py-8">
          <div className="mb-8 flex items-start justify-between">
            <div>
              <div className="flex items-center gap-3 mb-2">
                <Settings className="w-6 h-6 text-blue-400" />
                <h1 className="text-2xl font-semibold text-white">System Prompt</h1>
              </div>
              <p className="text-gray-400 text-sm">
                Applied to all scenario chats and test runs.
              </p>
            </div>
            {isDirty && (
              <button onClick={save} className="flex items-center gap-2 px-4 py-2 bg-blue-600 hover:bg-blue-500 text-white text-sm rounded-lg transition-colors">
                <Save className="w-4 h-4" />
                Save
              </button>
            )}
          </div>

          <textarea
            value={systemPrompt}
            onChange={(e) => handleChange(e.target.value)}
            onBlur={() => isDirty && save()}
            placeholder="You are a helpful assistant..."
            className="w-full bg-gray-800 border border-gray-700 text-gray-200 text-sm rounded-xl px-4 py-3 focus:outline-none focus:border-blue-500 resize-none leading-relaxed min-h-[500px]"
          />
        </div>
      </div>
    </div>
  );
}

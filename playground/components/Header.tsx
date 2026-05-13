// Copyright Qualcomm Innovation Center, Inc. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

"use client";

import { useRouter, usePathname } from "next/navigation";
import { useState } from "react";
import {
  Package,
  Zap,
  Settings,
  Plus,
  Key,
  X,
  Eye,
  EyeOff,
} from "lucide-react";
import { useSessionStore } from "@/store/sessionStore";
import { MODEL_OPTIONS } from "@/lib/types";
import { cn } from "@/lib/utils";

function ApiKeyModal({ onClose }: { onClose: () => void }) {
  const [apiKey, setApiKey] = useState(() => {
    if (typeof window !== "undefined") {
      return localStorage.getItem("gemini_api_key") ?? "";
    }
    return "";
  });
  const [show, setShow] = useState(false);
  const [saved, setSaved] = useState(false);

  const handleSave = () => {
    localStorage.setItem("gemini_api_key", apiKey.trim());
    setSaved(true);
    setTimeout(() => {
      setSaved(false);
      onClose();
    }, 1000);
  };

  return (
    <div
      className="fixed inset-0 z-50 flex items-center justify-center bg-black/60 backdrop-blur-sm"
      onClick={onClose}
    >
      <div
        className="bg-gray-800 border border-gray-700 rounded-xl shadow-2xl w-full max-w-md mx-4 p-6 space-y-4"
        onClick={(e) => e.stopPropagation()}
      >
        <div className="flex items-center justify-between">
          <h2 className="text-white font-semibold flex items-center gap-2">
            <Key className="w-4 h-4 text-blue-400" />
            Gemini API Key
          </h2>
          <button
            onClick={onClose}
            className="text-gray-400 hover:text-white transition-colors"
          >
            <X className="w-5 h-5" />
          </button>
        </div>

        <p className="text-xs text-gray-400">
          Enter your Gemini API key. It is stored only in your browser&apos;s
          local storage and sent directly to the API.
        </p>

        <div className="relative">
          <input
            type={show ? "text" : "password"}
            value={apiKey}
            onChange={(e) => setApiKey(e.target.value)}
            placeholder="AIza..."
            className="w-full bg-gray-900 border border-gray-700 rounded-lg px-3 py-2 pr-10 text-white text-sm font-mono placeholder-gray-500 focus:outline-none focus:border-blue-500"
          />
          <button
            type="button"
            onClick={() => setShow((v) => !v)}
            className="absolute right-2.5 top-1/2 -translate-y-1/2 text-gray-500 hover:text-gray-300 transition-colors"
          >
            {show ? <EyeOff className="w-4 h-4" /> : <Eye className="w-4 h-4" />}
          </button>
        </div>

        <div className="flex justify-end gap-3">
          <button
            onClick={onClose}
            className="px-4 py-2 text-sm text-gray-400 hover:text-white transition-colors"
          >
            Cancel
          </button>
          <button
            onClick={handleSave}
            className="px-4 py-2 bg-blue-600 hover:bg-blue-500 text-white text-sm rounded-lg transition-colors"
          >
            {saved ? "Saved!" : "Save"}
          </button>
        </div>
      </div>
    </div>
  );
}

export default function Header() {
  const router = useRouter();
  const pathname = usePathname();
  const { activeSession, createSession, updateSession } = useSessionStore();
  const [showApiKey, setShowApiKey] = useState(false);

  const handleNewSession = async () => {
    const session = await createSession();
    if (session) router.push("/sessions");
  };

  const handleModelChange = async (model: string) => {
    if (activeSession) await updateSession(activeSession.id, { model });
  };

  return (
    <header className="h-14 bg-gray-900 border-b border-gray-700 flex items-center justify-between px-4 flex-shrink-0">
      <div className="flex items-center gap-3">
        <div className="flex items-center gap-2">
          <span className="font-semibold text-white text-sm tracking-wide">
            MindSand Playground
          </span>
        </div>

        <div className="h-5 w-px bg-gray-700" />

        <nav className="flex items-center gap-1">
          {[
            { path: "/sessions", label: "Sessions" },
            { path: "/actions", label: "Actions", icon: <Zap className="w-3.5 h-3.5" /> },
            { path: "/objects", label: "Objects", icon: <Package className="w-3.5 h-3.5" /> },
            { path: "/config", label: "System Prompt", icon: <Settings className="w-3.5 h-3.5" /> },
          ].map(({ path, label, icon }) => (
            <button
              key={path}
              onClick={() => router.push(path)}
              className={cn(
                "px-3 py-1.5 rounded text-xs font-medium transition-colors flex items-center gap-1.5",
                pathname === path
                  ? "bg-gray-700 text-white"
                  : "text-gray-400 hover:text-white hover:bg-gray-800"
              )}
            >
              {icon}
              {label}
            </button>
          ))}
        </nav>
      </div>

      <div className="flex items-center gap-2">
        {activeSession && pathname === "/sessions" && (
          <select
            value={activeSession.model}
            onChange={(e) => handleModelChange(e.target.value)}
            className="bg-gray-800 border border-gray-600 text-gray-200 text-xs rounded px-2 py-1.5 focus:outline-none focus:border-blue-500 cursor-pointer"
          >
            {MODEL_OPTIONS.map((model) => (
              <option key={model.id} value={model.id}>
                {model.name}
              </option>
            ))}
          </select>
        )}

        <button
          onClick={handleNewSession}
          className="flex items-center gap-1.5 px-3 py-1.5 bg-blue-600 hover:bg-blue-500 text-white text-xs font-medium rounded transition-colors"
        >
          <Plus className="w-3.5 h-3.5" />
          New Session
        </button>

        <button
          onClick={() => setShowApiKey(true)}
          title="Set Gemini API Key"
          className="flex items-center gap-1.5 px-3 py-1.5 rounded text-xs font-medium transition-colors border bg-gray-700 text-gray-300 border-gray-600 hover:bg-gray-600"
        >
          <Key className="w-3.5 h-3.5" />
          API Key
        </button>
      </div>

      {showApiKey && <ApiKeyModal onClose={() => setShowApiKey(false)} />}
    </header>
  );
}
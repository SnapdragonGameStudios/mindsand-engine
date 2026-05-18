// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

"use client";

import { create } from "zustand";
import { Session, SessionSummary, Action, Message } from "@/lib/types";

interface SessionStore {
  sessions: SessionSummary[];
  activeSessionId: string | null;
  activeSession: Session | null;
  isLoadingSessions: boolean;
  isLoadingSession: boolean;
  isSendingMessage: boolean;
  streamingContent: string;
  error: string | null;

  // Actions
  fetchSessions: () => Promise<void>;
  createSession: () => Promise<Session | null>;
  loadSession: (id: string) => Promise<void>;
  updateSession: (id: string, data: Partial<Session>) => Promise<void>;
  deleteSession: (id: string) => Promise<void>;
  sendMessage: (content: string) => Promise<void>;
  addAction: (action: Omit<Action, "id" | "sessionId" | "createdAt">) => Promise<void>;
  updateAction: (id: string, data: Partial<Action>) => Promise<void>;
  deleteAction: (id: string) => Promise<void>;
  setError: (error: string | null) => void;
  clearMessages: () => void;
}

export const useSessionStore = create<SessionStore>((set, get) => ({
  sessions: [],
  activeSessionId: null,
  activeSession: null,
  isLoadingSessions: false,
  isLoadingSession: false,
  isSendingMessage: false,
  streamingContent: "",
  error: null,

  setError: (error) => set({ error }),
  clearMessages: () =>
    set((state) => ({
      activeSession: state.activeSession
        ? { ...state.activeSession, messages: [] }
        : null,
    })),

  fetchSessions: async () => {
    set({ isLoadingSessions: true, error: null });
    try {
      const res = await fetch("/api/sessions");
      if (!res.ok) throw new Error("Failed to fetch sessions");
      const data = await res.json();
      set({ sessions: data, isLoadingSessions: false });
    } catch (err) {
      set({
        error: err instanceof Error ? err.message : "Unknown error",
        isLoadingSessions: false,
      });
    }
  },

  createSession: async () => {
    try {
      // Fetch library actions to pre-populate the session
      const actionsRes = await fetch("/api/action-library");
      const libraryActions: { name: string; description: string; parameters: { name: string; type: string }[] }[] =
        actionsRes.ok ? await actionsRes.json() : [];

      const defaultGameState = JSON.stringify({
        player: { id: 1, position: [0, 0, 90] },
      });

      // Delete all existing sessions first (only one session at a time)
      const existingSessions = get().sessions;
      for (const s of existingSessions) {
        await fetch(`/api/sessions/${s.id}`, { method: "DELETE" });
      }

      // Create session with default game state
      const res = await fetch("/api/sessions", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          name: "New Session",
          model: "gemini-3-flash-preview",
          gameState: defaultGameState,
        }),
      });
      if (!res.ok) throw new Error("Failed to create session");
      const session = await res.json();

      // Pre-populate session actions from library
      const createdActions = [];
      for (const la of libraryActions) {
        const actionRes = await fetch("/api/actions", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({
            sessionId: session.id,
            actionName: la.name,
            parameters: JSON.stringify(
              Object.fromEntries(la.parameters.map((p) => [p.name, p.type]))
            ),
            description: la.description,
          }),
        });
        if (actionRes.ok) createdActions.push(await actionRes.json());
      }

      const fullSession = { ...session, actions: createdActions };
      set((state) => ({
        sessions: [fullSession, ...state.sessions],
        activeSessionId: fullSession.id,
        activeSession: fullSession,
      }));
      return fullSession;
    } catch (err) {
      set({ error: err instanceof Error ? err.message : "Unknown error" });
      return null;
    }
  },

  loadSession: async (id: string) => {
    set({ isLoadingSession: true, error: null });
    try {
      const res = await fetch(`/api/sessions/${id}`);
      if (!res.ok) throw new Error("Failed to load session");
      const session = await res.json();
      set({
        activeSessionId: id,
        activeSession: session,
        isLoadingSession: false,
      });
    } catch (err) {
      set({
        error: err instanceof Error ? err.message : "Unknown error",
        isLoadingSession: false,
      });
    }
  },

  updateSession: async (id: string, data: Partial<Session>) => {
    try {
      const res = await fetch(`/api/sessions/${id}`, {
        method: "PUT",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(data),
      });
      if (!res.ok) throw new Error("Failed to update session");
      const updated = await res.json();
      set((state) => ({
        activeSession:
          state.activeSession?.id === id
            ? { ...state.activeSession, ...updated }
            : state.activeSession,
        sessions: state.sessions.map((s) =>
          s.id === id ? { ...s, ...updated } : s
        ),
      }));
    } catch (err) {
      set({ error: err instanceof Error ? err.message : "Unknown error" });
    }
  },

  deleteSession: async (id: string) => {
    try {
      const res = await fetch(`/api/sessions/${id}`, { method: "DELETE" });
      if (!res.ok) throw new Error("Failed to delete session");
      set((state) => {
        const newSessions = state.sessions.filter((s) => s.id !== id);
        return {
          sessions: newSessions,
          activeSessionId:
            state.activeSessionId === id ? null : state.activeSessionId,
          activeSession:
            state.activeSession?.id === id ? null : state.activeSession,
        };
      });
    } catch (err) {
      set({ error: err instanceof Error ? err.message : "Unknown error" });
    }
  },

  sendMessage: async (content: string) => {
    const { activeSession } = get();
    if (!activeSession) return;

    const userMessage: Message = {
      id: `temp-${Date.now()}`,
      sessionId: activeSession.id,
      role: "user",
      content,
      timestamp: new Date().toISOString(),
    };

    set((state) => ({
      isSendingMessage: true,
      streamingContent: "",
      error: null,
      activeSession: state.activeSession
        ? {
            ...state.activeSession,
            messages: [...state.activeSession.messages, userMessage],
          }
        : null,
    }));

    try {
      const geminiApiKey =
        typeof window !== "undefined"
          ? localStorage.getItem("gemini_api_key") ?? ""
          : "";

      const chatHeaders: Record<string, string> = { "Content-Type": "application/json" };
      if (geminiApiKey) chatHeaders["x-gemini-api-key"] = geminiApiKey;

      const res = await fetch("/api/chat", {
        method: "POST",
        headers: chatHeaders,
        body: JSON.stringify({
          sessionId: activeSession.id,
          message: content,
        }),
      });

      const data = await res.json();
      if (!res.ok) throw new Error(data.error || "Failed to send message");

      set((state) => ({
        isSendingMessage: false,
        streamingContent: "",
        activeSession: state.activeSession
          ? {
              ...state.activeSession,
              messages: [
                ...state.activeSession.messages.filter(
                  (m) => !m.id.startsWith("temp-")
                ),
                data.userMessage,
                data.message,
              ],
            }
          : null,
      }));
    } catch (err) {
      set((state) => ({
        isSendingMessage: false,
        streamingContent: "",
        error: err instanceof Error ? err.message : "Unknown error",
        activeSession: state.activeSession
          ? {
              ...state.activeSession,
              messages: state.activeSession.messages.filter(
                (m) => !m.id.startsWith("temp-")
              ),
            }
          : null,
      }));
    }
  },

  addAction: async (actionData) => {
    const { activeSession } = get();
    if (!activeSession) return;

    try {
      const res = await fetch("/api/actions", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          sessionId: activeSession.id,
          ...actionData,
        }),
      });
      if (!res.ok) throw new Error("Failed to add action");
      const action = await res.json();
      set((state) => ({
        activeSession: state.activeSession
          ? {
              ...state.activeSession,
              actions: [...state.activeSession.actions, action],
            }
          : null,
      }));
    } catch (err) {
      set({ error: err instanceof Error ? err.message : "Unknown error" });
    }
  },

  updateAction: async (id: string, data: Partial<Action>) => {
    try {
      const res = await fetch(`/api/actions/${id}`, {
        method: "PUT",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(data),
      });
      if (!res.ok) throw new Error("Failed to update action");
      const updated = await res.json();
      set((state) => ({
        activeSession: state.activeSession
          ? {
              ...state.activeSession,
              actions: state.activeSession.actions.map((a) =>
                a.id === id ? { ...a, ...updated } : a
              ),
            }
          : null,
      }));
    } catch (err) {
      set({ error: err instanceof Error ? err.message : "Unknown error" });
    }
  },

  deleteAction: async (id: string) => {
    try {
      const res = await fetch(`/api/actions/${id}`, { method: "DELETE" });
      if (!res.ok) throw new Error("Failed to delete action");
      set((state) => ({
        activeSession: state.activeSession
          ? {
              ...state.activeSession,
              actions: state.activeSession.actions.filter((a) => a.id !== id),
            }
          : null,
      }));
    } catch (err) {
      set({ error: err instanceof Error ? err.message : "Unknown error" });
    }
  },
}));

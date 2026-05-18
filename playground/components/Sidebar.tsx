// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

"use client";

import { useEffect } from "react";
import { MessageSquare } from "lucide-react";
import { useSessionStore } from "@/store/sessionStore";
import { formatDate } from "@/lib/utils";

export default function Sidebar() {
  const {
    sessions,
    activeSession,
    isLoadingSessions,
    fetchSessions,
    loadSession,
  } = useSessionStore();

  useEffect(() => {
    fetchSessions();
  }, [fetchSessions]);

  // Auto-load the single session if one exists and none is active
  useEffect(() => {
    if (!activeSession && sessions.length > 0) {
      loadSession(sessions[0].id);
    }
  }, [sessions, activeSession, loadSession]);

  const session = activeSession ?? (sessions.length > 0 ? sessions[0] : null);

  return (
    <div className="w-[200px] flex-shrink-0 bg-gray-800 border-r border-gray-700 flex flex-col">
      <div className="p-3 border-b border-gray-700">
        <span className="text-xs font-semibold text-gray-400 uppercase tracking-wider">
          Session
        </span>
      </div>

      <div className="flex-1 overflow-y-auto">
        {isLoadingSessions ? (
          <div className="p-4 text-center">
            <div className="text-gray-500 text-xs">Loading...</div>
          </div>
        ) : !session ? (
          <div className="p-4 text-center">
            <MessageSquare className="w-8 h-8 text-gray-600 mx-auto mb-2" />
            <p className="text-gray-500 text-xs">No session yet</p>
            <p className="text-gray-600 text-xs mt-1">
              Click &quot;New Session&quot; to start
            </p>
          </div>
        ) : (
          <div className="py-1">
            <div className="flex items-start gap-2 px-3 py-2.5 bg-gray-700/60 border-l-2 border-blue-500 text-white">
              <MessageSquare className="w-3.5 h-3.5 mt-0.5 flex-shrink-0 text-blue-400" />
              <div className="flex-1 min-w-0">
                <p className="text-xs font-medium truncate leading-tight">
                  {session.name}
                </p>
                <p className="text-xs text-gray-500 mt-0.5 truncate">
                  {formatDate(session.updatedAt)}
                </p>
                {"_count" in session && session._count && (
                  <p className="text-xs text-gray-600 mt-0.5">
                    {(session._count as { messages: number }).messages} msg
                    {(session._count as { messages: number }).messages !== 1 ? "s" : ""}
                  </p>
                )}
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
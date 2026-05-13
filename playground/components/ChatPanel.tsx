// Copyright Qualcomm Innovation Center, Inc. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

"use client";

import { useEffect, useRef, useState } from "react";
import { Send, Bot, User, AlertCircle, Loader2, Pencil, Check } from "lucide-react";
import { useSessionStore } from "@/store/sessionStore";
import { cn, formatDate } from "@/lib/utils";

function TypingIndicator() {
  return (
    <div className="flex items-center gap-1.5 px-3 py-2">
      <div className="w-1.5 h-1.5 bg-gray-400 rounded-full animate-bounce [animation-delay:-0.3s]" />
      <div className="w-1.5 h-1.5 bg-gray-400 rounded-full animate-bounce [animation-delay:-0.15s]" />
      <div className="w-1.5 h-1.5 bg-gray-400 rounded-full animate-bounce" />
    </div>
  );
}

function MessageContent({ content }: { content: string }) {
  // Simple markdown-like rendering for code blocks
  const parts = content.split(/(```[\s\S]*?```)/g);

  return (
    <div className="text-sm leading-relaxed whitespace-pre-wrap break-words">
      {parts.map((part, i) => {
        if (part.startsWith("```") && part.endsWith("```")) {
          const lines = part.slice(3, -3).split("\n");
          const lang = lines[0];
          const code = lines.slice(1).join("\n");
          return (
            <div key={i} className="my-2">
              {lang && (
                <div className="text-xs text-gray-500 bg-gray-800 px-3 py-1 rounded-t border border-gray-700 border-b-0 font-mono">
                  {lang}
                </div>
              )}
              <pre className={cn(
                "bg-gray-800 border border-gray-700 p-3 overflow-x-auto text-xs font-mono text-gray-300",
                lang ? "rounded-b rounded-tr" : "rounded"
              )}>
                <code>{code}</code>
              </pre>
            </div>
          );
        }
        return <span key={i}>{part}</span>;
      })}
    </div>
  );
}

export default function ChatPanel() {
  const {
    activeSession,
    isSendingMessage,
    streamingContent,
    error,
    setError,
    updateSession,
  } = useSessionStore();

  const [input, setInput] = useState("");
  const [isEditingName, setIsEditingName] = useState(false);
  const [sessionName, setSessionName] = useState("");
  const messagesEndRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLTextAreaElement>(null);
  const { sendMessage } = useSessionStore();

  useEffect(() => {
    if (activeSession) {
      setSessionName(activeSession.name);
    }
  }, [activeSession?.id]);

  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [activeSession?.messages, streamingContent]);

  const handleSend = async () => {
    if (!input.trim() || isSendingMessage || !activeSession) return;
    const message = input.trim();
    setInput("");
    await sendMessage(message);
    inputRef.current?.focus();
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === "Enter" && !e.shiftKey) {
      e.preventDefault();
      handleSend();
    }
  };

  const handleSaveName = async () => {
    if (activeSession && sessionName.trim()) {
      await updateSession(activeSession.id, { name: sessionName.trim() });
    }
    setIsEditingName(false);
  };

  if (!activeSession) {
    return (
      <div className="flex-1 flex flex-col items-center justify-center bg-gray-900">
        <Bot className="w-16 h-16 text-gray-700 mb-4" />
        <h2 className="text-gray-400 text-lg font-medium mb-2">
          LLM Interaction Platform
        </h2>
        <p className="text-gray-600 text-sm text-center max-w-sm">
          Select an existing session or create a new one to start chatting with
          an LLM.
        </p>
      </div>
    );
  }

  const messages = activeSession.messages || [];

  return (
    <div className="flex-1 flex flex-col bg-gray-900 min-w-0">
      {/* Chat Header */}
      <div className="h-12 border-b border-gray-700 flex items-center px-4 gap-3 flex-shrink-0">
        {isEditingName ? (
          <div className="flex items-center gap-2 flex-1">
            <input
              autoFocus
              value={sessionName}
              onChange={(e) => setSessionName(e.target.value)}
              onKeyDown={(e) => {
                if (e.key === "Enter") handleSaveName();
                if (e.key === "Escape") setIsEditingName(false);
              }}
              className="bg-gray-800 border border-blue-500 text-white text-sm rounded px-2 py-0.5 focus:outline-none"
            />
            <button
              onClick={handleSaveName}
              className="p-1 text-green-400 hover:text-green-300"
            >
              <Check className="w-4 h-4" />
            </button>
          </div>
        ) : (
          <div className="flex items-center gap-2 flex-1 min-w-0">
            <h2 className="text-sm font-medium text-gray-200 truncate">
              {activeSession.name}
            </h2>
            <button
              onClick={() => setIsEditingName(true)}
              className="p-1 text-gray-600 hover:text-gray-400 flex-shrink-0"
            >
              <Pencil className="w-3 h-3" />
            </button>
          </div>
        )}
        <span className="text-xs text-gray-600 flex-shrink-0">
          {messages.length} message{messages.length !== 1 ? "s" : ""}
        </span>
      </div>

      {/* Messages */}
      <div className="flex-1 overflow-y-auto px-4 py-4 space-y-4">
        {error && (
          <div className="flex items-start gap-2 p-3 bg-red-900/20 border border-red-700/30 rounded-lg">
            <AlertCircle className="w-4 h-4 text-red-400 mt-0.5 flex-shrink-0" />
            <div className="flex-1 min-w-0">
              <p className="text-sm text-red-300">{error}</p>
              <button
                onClick={() => setError(null)}
                className="text-xs text-red-400 hover:text-red-300 mt-1"
              >
                Dismiss
              </button>
            </div>
          </div>
        )}

        {messages.length === 0 && !isSendingMessage && (
          <div className="flex flex-col items-center justify-center h-full min-h-[300px] text-center">
            <Bot className="w-12 h-12 text-gray-700 mb-3" />
            <p className="text-gray-500 text-sm">
              Start the conversation by typing a message below.
            </p>
            <p className="text-gray-600 text-xs mt-1">
              Configure the system prompt and scenario in the right panel.
            </p>
          </div>
        )}

        {messages.map((message) => (
          <div
            key={message.id}
            className={cn(
              "flex gap-3 max-w-full",
              message.role === "user" ? "flex-row-reverse" : "flex-row"
            )}
          >
            <div
              className={cn(
                "w-7 h-7 rounded-full flex items-center justify-center flex-shrink-0 mt-0.5",
                message.role === "user"
                  ? "bg-blue-600"
                  : "bg-gray-700"
              )}
            >
              {message.role === "user" ? (
                <User className="w-3.5 h-3.5 text-white" />
              ) : (
                <Bot className="w-3.5 h-3.5 text-gray-300" />
              )}
            </div>

            <div
              className={cn(
                "flex-1 min-w-0 max-w-[80%]",
                message.role === "user" ? "items-end" : "items-start",
                "flex flex-col"
              )}
            >
              <div
                className={cn(
                  "rounded-2xl px-4 py-3",
                  message.role === "user"
                    ? "bg-blue-600 text-white rounded-tr-sm"
                    : "bg-gray-800 text-gray-100 rounded-tl-sm"
                )}
              >
                <MessageContent content={message.content} />
              </div>
              <span
                className={cn(
                  "text-xs text-gray-600 mt-1 px-1",
                  message.role === "user" ? "text-right" : "text-left"
                )}
              >
                {formatDate(message.timestamp)}
              </span>
            </div>
          </div>
        ))}

        {/* Streaming message */}
        {isSendingMessage && streamingContent && (
          <div className="flex gap-3 max-w-full">
            <div className="w-7 h-7 rounded-full bg-gray-700 flex items-center justify-center flex-shrink-0 mt-0.5">
              <Bot className="w-3.5 h-3.5 text-gray-300" />
            </div>
            <div className="flex-1 min-w-0 max-w-[80%]">
              <div className="bg-gray-800 text-gray-100 rounded-2xl rounded-tl-sm px-4 py-3">
                <MessageContent content={streamingContent} />
                <span className="inline-block w-1 h-4 bg-blue-400 animate-pulse ml-0.5 align-middle" />
              </div>
            </div>
          </div>
        )}

        {/* Loading indicator (before first token) */}
        {isSendingMessage && !streamingContent && (
          <div className="flex gap-3">
            <div className="w-7 h-7 rounded-full bg-gray-700 flex items-center justify-center flex-shrink-0">
              <Bot className="w-3.5 h-3.5 text-gray-300" />
            </div>
            <div className="bg-gray-800 rounded-2xl rounded-tl-sm">
              <TypingIndicator />
            </div>
          </div>
        )}

        <div ref={messagesEndRef} />
      </div>

      {/* Input Area */}
      <div className="border-t border-gray-700 p-4 flex-shrink-0">
        <div className="flex gap-3 items-end">
          <div className="flex-1 relative">
            <textarea
              ref={inputRef}
              value={input}
              onChange={(e) => setInput(e.target.value)}
              onKeyDown={handleKeyDown}
              placeholder="Type a message... (Enter to send, Shift+Enter for newline)"
              disabled={isSendingMessage}
              rows={1}
              className="w-full bg-gray-800 border border-gray-600 text-gray-200 text-sm rounded-xl px-4 py-3 focus:outline-none focus:border-blue-500 resize-none disabled:opacity-50 leading-relaxed"
              style={{ maxHeight: "200px", overflow: "auto" }}
              onInput={(e) => {
                const target = e.target as HTMLTextAreaElement;
                target.style.height = "auto";
                target.style.height = `${Math.min(target.scrollHeight, 200)}px`;
              }}
            />
          </div>
          <button
            onClick={handleSend}
            disabled={!input.trim() || isSendingMessage}
            className={cn(
              "w-10 h-10 rounded-xl flex items-center justify-center flex-shrink-0 transition-all",
              input.trim() && !isSendingMessage
                ? "bg-blue-600 hover:bg-blue-500 text-white"
                : "bg-gray-700 text-gray-500 cursor-not-allowed"
            )}
          >
            {isSendingMessage ? (
              <Loader2 className="w-4 h-4 animate-spin" />
            ) : (
              <Send className="w-4 h-4" />
            )}
          </button>
        </div>
        <p className="text-xs text-gray-600 mt-2 text-center">
          Model: {activeSession.model}
        </p>
      </div>
    </div>
  );
}

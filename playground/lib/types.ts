// Copyright Qualcomm Innovation Center, Inc. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

export interface Action {
  id: string;
  sessionId: string;
  actionName: string;
  parameters: string; // JSON string
  description: string;
  createdAt: string;
}

export interface Message {
  id: string;
  sessionId: string;
  role: "user" | "assistant";
  content: string;
  timestamp: string;
}

export interface Session {
  id: string;
  name: string;
  model: string;
  systemPrompt: string;
  scenario: string;
  gameState: string; // JSON string
  perceptionState: string; // JSON string
  actions: Action[];
  messages: Message[];
  createdAt: string;
  updatedAt: string;
}

export interface SessionSummary {
  id: string;
  name: string;
  model: string;
  createdAt: string;
  updatedAt: string;
  _count?: {
    messages: number;
  };
}

export interface GameObject {
  id: string;
  objectId: string;  // user-defined unique identifier (sent in perception JSON)
  name: string;      // internal display label (not sent to LLM)
  description: string;
  location: { x: number; y: number; z: number };
  createdAt: string;
}

export interface LibraryActionParam {
  name: string;
  type: "Float" | "String" | "Actor";
}

export interface LibraryAction {
  id: string;
  name: string;
  description: string;
  parameters: LibraryActionParam[];
  createdAt: string;
}

export type ModelOption = {
  id: string;
  name: string;
};

export const MODEL_OPTIONS: ModelOption[] = [
  { id: "gemini-3-flash-preview", name: "Gemini 3 Flash Preview" },
  { id: "gemini-3.1-flash-lite-preview", name: "Gemini 3.1 Flash Lite Preview" },
];

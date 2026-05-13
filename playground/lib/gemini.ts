import { Action } from "./types";

export function buildSystemPrompt(
  systemPrompt: string,
  scenario: string
): string {
  const parts: string[] = [];

  if (systemPrompt?.trim()) {
    parts.push(systemPrompt.trim());
  }

  if (scenario?.trim()) {
    parts.push(`\nScenario: ${scenario.trim()}`);
  }

  return parts.join("\n");
}

export function buildPrompt(
  gameState: string,
  perceptionState: string,
  message: string,
  userHistory: string[] = [],
  actions: Action[] = []
): string {
  const parts: string[] = [gameState, perceptionState];

  if (actions.length > 0) {
    const actionsJson = actions.map((action) => {
      let params: Record<string, unknown> = {};
      try {
        params = JSON.parse(action.parameters || "{}");
      } catch {
        // ignore
      }
      return { actionName: action.actionName, parameters: params, description: action.description };
    });
    parts.push(`Available actions: ${JSON.stringify(actionsJson, null, 2)}`);
  }

  if (userHistory.length > 0) {
    parts.push(`User history: ${userHistory.join(", ")}`);
  }

  parts.push(`prompt: ${message}`);
  return parts.join("\n");
}

import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { getUserFromRequest } from "@/lib/apiHelpers";
import { buildSystemPrompt, buildPrompt } from "@/lib/gemini";
import { GoogleGenerativeAI } from "@google/generative-ai";

export async function POST(request: NextRequest) {
  try {
    const user = await getUserFromRequest(request);
    const body = await request.json();
    const { sessionId, message, gameState: bodyGameState, perceptionState: bodyPerceptionState } = body;

    if (!sessionId || !message) {
      return NextResponse.json({ error: "sessionId and message are required" }, { status: 400 });
    }

    const session = await prisma.session.findFirst({
      where: { id: sessionId, ownerId: user.userId },
    });
    if (!session) return NextResponse.json({ error: "Session not found" }, { status: 404 });

    const actions = await prisma.action.findMany({
      where: { sessionId },
      orderBy: { createdAt: "asc" },
    });

    const userHistory = await prisma.message.findMany({
      where: { sessionId, role: "user" },
      orderBy: { timestamp: "asc" },
      select: { content: true },
    });

    const globalConfig = await prisma.globalConfig.findUnique({ where: { id: "singleton" } });
    const systemPrompt = buildSystemPrompt(globalConfig?.systemPrompt ?? "", session.scenario);

    const prompt = buildPrompt(
      bodyGameState ?? session.gameState,
      bodyPerceptionState ?? session.perceptionState,
      message,
      userHistory.map((m: { content: string }) => m.content),
      actions.map((a: { id: string; sessionId: string; actionName: string; parameters: string; description: string; createdAt: Date }) => ({
        id: a.id, sessionId: a.sessionId, actionName: a.actionName,
        parameters: a.parameters, description: a.description, createdAt: a.createdAt.toISOString(),
      }))
    );

    // Resolve API key: prefer header from client, fall back to env var
    const apiKey =
      request.headers.get("x-gemini-api-key") ||
      process.env.GEMINI_API_KEY;

    if (!apiKey) {
      return NextResponse.json(
        { error: "No Gemini API key configured. Please set your API key via the API Key button." },
        { status: 400 }
      );
    }

    // Call Gemini directly using the SDK
    const genAI = new GoogleGenerativeAI(apiKey);
    const VALID_MODELS = ["gemini-3-flash-preview", "gemini-3.1-flash-lite-preview"];
    const modelId = VALID_MODELS.includes(session.model) ? session.model : "gemini-3-flash-preview";

    // Auto-migrate stale model IDs in the database
    if (modelId !== session.model) {
      await prisma.session.update({ where: { id: sessionId }, data: { model: modelId } });
    }

    const model = genAI.getGenerativeModel({
      model: modelId,
      systemInstruction: systemPrompt || undefined,
    });

    const result = await model.generateContent(prompt);
    const responseText = result.response.text();

    const [savedUserMessage, savedAssistantMessage] = await prisma.$transaction([
      prisma.message.create({ data: { sessionId, role: "user", content: message } }),
      prisma.message.create({ data: { sessionId, role: "assistant", content: responseText } }),
    ]);

    await prisma.session.update({ where: { id: sessionId }, data: { updatedAt: new Date() } });

    return NextResponse.json({
      userMessage: { ...savedUserMessage, timestamp: savedUserMessage.timestamp.toISOString() },
      message: { ...savedAssistantMessage, timestamp: savedAssistantMessage.timestamp.toISOString() },
    });
  } catch (error) {
    console.error("POST /api/chat error:", error);
    return NextResponse.json({ error: error instanceof Error ? error.message : "Failed to process chat" }, { status: 500 });
  }
}
import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { getUserFromRequest } from "@/lib/apiHelpers";

export async function GET(request: NextRequest) {
  try {
    const user = await getUserFromRequest(request);
    const sessions = await prisma.session.findMany({
      where: { ownerId: user.userId },
      orderBy: { updatedAt: "desc" },
      include: { _count: { select: { messages: true } } },
    });
    return NextResponse.json(
      sessions.map((s: { id: string; name: string; model: string; createdAt: Date; updatedAt: Date; _count: { messages: number } }) => ({
        id: s.id, name: s.name, model: s.model,
        createdAt: s.createdAt.toISOString(), updatedAt: s.updatedAt.toISOString(),
        _count: s._count,
      }))
    );
  } catch (error) {
    console.error("GET /api/sessions error:", error);
    return NextResponse.json({ error: "Failed to fetch sessions" }, { status: 500 });
  }
}

export async function POST(request: NextRequest) {
  try {
    const user = await getUserFromRequest(request);
    const body = await request.json();
    const session = await prisma.session.create({
      data: {
        ownerId: user.userId,
        name: body.name || "New Session",
        model: body.model || "gemini-2.0-flash",
        systemPrompt: body.systemPrompt || "",
        scenario: body.scenario || "",
        gameState: body.gameState || "{}",
        perceptionState: body.perceptionState || "{}",
      },
    });
    return NextResponse.json({
      ...session, actions: [], messages: [],
      createdAt: session.createdAt.toISOString(), updatedAt: session.updatedAt.toISOString(),
    }, { status: 201 });
  } catch (error) {
    console.error("POST /api/sessions error:", error);
    return NextResponse.json({ error: "Failed to create session" }, { status: 500 });
  }
}
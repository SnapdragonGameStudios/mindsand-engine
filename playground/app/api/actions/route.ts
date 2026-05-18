// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { getUserFromRequest } from "@/lib/apiHelpers";

export async function GET(request: NextRequest) {
  try {
    const user = await getUserFromRequest(request);
    const { searchParams } = new URL(request.url);
    const sessionId = searchParams.get("sessionId");

    if (!sessionId) {
      const actions = await prisma.action.findMany({
        where: { session: { ownerId: user.userId } },
        orderBy: { createdAt: "asc" },
      });
      const seen = new Set<string>();
      type ActionRow = { actionName: string; createdAt: Date; [k: string]: unknown };
      const unique = (actions as ActionRow[]).filter((a) => {
        if (seen.has(a.actionName)) return false;
        seen.add(a.actionName);
        return true;
      });
      return NextResponse.json(unique.map((a) => ({ ...a, createdAt: a.createdAt.toISOString() })));
    }

    const actions = await prisma.action.findMany({
      where: { sessionId },
      orderBy: { createdAt: "asc" },
    });
    return NextResponse.json(
      (actions as { createdAt: Date; [k: string]: unknown }[]).map((a) => ({ ...a, createdAt: a.createdAt.toISOString() }))
    );
  } catch (error) {
    console.error("GET /api/actions error:", error);
    return NextResponse.json({ error: "Failed to fetch actions" }, { status: 500 });
  }
}

export async function POST(request: NextRequest) {
  try {
    const user = await getUserFromRequest(request);
    const body = await request.json();
    const { sessionId, actionName, parameters, description } = body;
    if (!sessionId || !actionName) {
      return NextResponse.json({ error: "sessionId and actionName are required" }, { status: 400 });
    }
    const action = await prisma.action.create({
      data: { sessionId, actionName, parameters: parameters || "{}", description: description || "" },
    });
    return NextResponse.json({ ...action, createdAt: action.createdAt.toISOString() }, { status: 201 });
  } catch (error) {
    console.error("POST /api/actions error:", error);
    return NextResponse.json({ error: "Failed to create action" }, { status: 500 });
  }
}
// Copyright Qualcomm Innovation Center, Inc. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

import { NextRequest, NextResponse } from "next/server";
import { prisma } from "@/lib/prisma";
import { getUserFromRequest } from "@/lib/apiHelpers";

export async function GET(request: NextRequest) {
  try {
    await getUserFromRequest(request);
    const config = await prisma.globalConfig.upsert({
      where: { id: "singleton" },
      update: {},
      create: { id: "singleton", systemPrompt: "" },
    });
    return NextResponse.json({ systemPrompt: config.systemPrompt });
  } catch (error) {
    console.error("GET /api/config error:", error);
    return NextResponse.json({ error: "Failed to fetch config" }, { status: 500 });
  }
}

export async function PUT(request: NextRequest) {
  try {
    await getUserFromRequest(request);
    const body = await request.json();
    const config = await prisma.globalConfig.upsert({
      where: { id: "singleton" },
      update: { systemPrompt: body.systemPrompt ?? "" },
      create: { id: "singleton", systemPrompt: body.systemPrompt ?? "" },
    });
    return NextResponse.json({ systemPrompt: config.systemPrompt });
  } catch (error) {
    console.error("PUT /api/config error:", error);
    return NextResponse.json({ error: "Failed to update config" }, { status: 500 });
  }
}
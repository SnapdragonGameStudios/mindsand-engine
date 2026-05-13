// Copyright Qualcomm Innovation Center, Inc. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

import { NextRequest, NextResponse } from "next/server";

export async function middleware(request: NextRequest) {
  const { pathname } = request.nextUrl;

  // Allow Next.js internals and static files
  if (
    pathname.startsWith("/_next") ||
    pathname.startsWith("/favicon") ||
    pathname.includes(".")
  ) {
    return NextResponse.next();
  }

  // Read or generate user ID
  const existingUserId = request.cookies.get("user_id")?.value;
  const userId = existingUserId ?? crypto.randomUUID();

  // Attach user ID to request headers for API routes
  const requestHeaders = new Headers(request.headers);
  requestHeaders.set("x-user-id", userId);

  const response = NextResponse.next({ request: { headers: requestHeaders } });

  // Set user_id cookie if new (1 year)
  if (!existingUserId) {
    response.cookies.set("user_id", userId, {
      httpOnly: true,
      sameSite: "lax",
      maxAge: 365 * 24 * 60 * 60,
      path: "/",
    });
  }

  return response;
}

export const config = {
  matcher: ["/((?!_next/static|_next/image|favicon.ico).*)"],
};
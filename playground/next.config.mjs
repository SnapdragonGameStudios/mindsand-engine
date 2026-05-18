// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

/** @type {import('next').NextConfig} */
const nextConfig = {
  experimental: {
    serverComponentsExternalPackages: [
      "@prisma/client",
      "@prisma/adapter-better-sqlite3",
      "better-sqlite3",
      "prisma",
    ],
  },
};

export default nextConfig;
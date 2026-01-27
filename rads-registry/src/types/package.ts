// TypeScript types for RADS Package Registry

export interface Package {
  name: string;
  displayName: string;
  description: string;
  version: string;
  author: string;
  repository: string;
  homepage?: string;
  keywords: string[];
  license: string;
  readme: string;
  latestVersion: string;
  versions: PackageVersion[];
  stats: PackageStats;
  category?: string;
}

export interface PackageVersion {
  version: string;
  releaseDate: string;
  releaseUrl: string;
  downloadUrl: string;
  checksum: string;
}

export interface PackageStats {
  downloads: number;
  stars: number;
  lastUpdated: string;
}

export interface User {
  login: string;
  name?: string;
  avatarUrl: string;
  bio?: string;
  url?: string;
}

export interface SearchFilters {
  query?: string;
  category?: string;
  license?: string;
  sort?: 'relevance' | 'downloads' | 'stars' | 'updated';
}

export interface ApiError {
  message: string;
  code?: number;
}

export interface ApiResponse<T> {
  data?: T;
  error?: ApiError;
  success: boolean;
}

export interface AuthSession {
  accessToken: string;
  user: User;
  expiresAt: number;
}

// Category types
export type PackageCategory =
  | 'database'
  | 'web'
  | 'utils'
  | 'media'
  | 'networking'
  | 'testing'
  | 'cli'
  | 'other';

// License types
export type PackageLicense =
  | 'MIT'
  | 'Apache-2.0'
  | 'GPL-3.0'
  | 'BSD-3'
  | 'ISC'
  | 'Other';

// Status types
export type PackageStatus = 'published' | 'yanked' | 'deprecated';

// Package list response (paginated)
export interface PackageListResponse {
  packages: Package[];
  total: number;
  page: number;
  pageSize: number;
}

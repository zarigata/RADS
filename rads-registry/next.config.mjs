const nextConfig = {
  output: 'export',
  images: {
    unoptimized: true,
  },
  trailingSlash: true,
  assetPrefix: '/RADS',
  env: {
    NEXT_PUBLIC_GITHUB_CLIENT_ID: process.env.NEXT_PUBLIC_GITHUB_CLIENT_ID,
    NEXT_PUBLIC_API_BASE: 'https://zarigata.github.io/RADS',
    NEXT_PUBLIC_REGISTRY_URL: 'https://zarigata.github.io/RADS',
  },
}

export default nextConfig

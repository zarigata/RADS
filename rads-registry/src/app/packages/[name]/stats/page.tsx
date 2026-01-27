import { StatsDisplay } from "@/components/package/StatsDisplay"
import { apiClient } from "@/lib/api"
import { Suspense } from "react"

export const dynamic = "force-dynamic"

interface StatsPageProps {
  params: {
    name: string
  }
}

export default async function StatsPage({ params }: StatsPageProps) {
  const response = await apiClient.getPackage(params.name)

  if (!response || !response.stats) {
    return (
      <div className="container py-16 text-center">
        <h1 className="text-2xl font-bold mb-4">Package not found</h1>
        <p className="text-muted-foreground">
          Package {params.name} not found in registry.
        </p>
      </div>
    )
  }

  return (
    <div className="container py-8">
      <div className="max-w-2xl mx-auto">
        <div className="mb-8">
          <h1 className="text-4xl font-bold mb-2">
            {response.displayName || response.name}
          </h1>
          <p className="text-muted-foreground text-lg">
            Download Statistics
          </p>
        </div>

        <Suspense fallback={<div className="h-64 bg-muted animate-pulse rounded-lg" />}>
          <StatsDisplay stats={response.stats} packageName={response.name} />
        </Suspense>

        <div className="mt-8 rounded-lg bg-muted/50 p-6">
          <h3 className="text-lg font-semibold mb-4">About These Stats</h3>
          <div className="space-y-3 text-sm text-muted-foreground">
            <p>
              <strong>Total Downloads:</strong> Number of times this package has been downloaded from GitHub Releases.
            </p>
            <p>
              <strong>GitHub Stars:</strong> Stars on the package's GitHub repository.
            </p>
            <p>
              <strong>Last Updated:</strong> When the package was last updated in the registry.
            </p>
            <p className="text-xs pt-4 border-t mt-4">
              Note: Statistics are updated periodically. Download counts may have a slight delay.
            </p>
          </div>
        </div>
      </div>
    </div>
  )
}

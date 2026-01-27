"use client"

import { useState, useEffect } from "react"
import { PackageCard } from "@/components/package/PackageCard"
import { SearchBar } from "@/components/package/SearchBar"
import { Button } from "@/components/ui/button"
import { Badge } from "@/components/ui/badge"
import { Package, SearchFilters, PackageCategory, PackageLicense } from "@/types/package"
import { Package, Filter, SlidersHorizontal } from "lucide-react"

const CATEGORIES: { value: PackageCategory; label: string }[] = [
  { value: "database", label: "Database" },
  { value: "web", label: "Web" },
  { value: "utils", label: "Utilities" },
  { value: "media", label: "Media" },
  { value: "networking", label: "Networking" },
  { value: "testing", label: "Testing" },
  { value: "cli", label: "CLI" },
  { value: "other", label: "Other" },
]

const LICENSES: { value: PackageLicense; label: string }[] = [
  { value: "MIT", label: "MIT" },
  { value: "Apache-2.0", label: "Apache 2.0" },
  { value: "GPL-3.0", label: "GPL 3.0" },
  { value: "BSD-3", label: "BSD-3" },
  { value: "ISC", label: "ISC" },
]

const SORT_OPTIONS = [
  { value: "relevance", label: "Relevance" },
  { value: "downloads", label: "Downloads" },
  { value: "stars", label: "Stars" },
  { value: "updated", label: "Recently Updated" },
]

export default function PackagesPage() {
  const [packages, setPackages] = useState<Package[]>([])
  const [loading, setLoading] = useState(true)
  const [filters, setFilters] = useState<SearchFilters>({})
  const [showFilters, setShowFilters] = useState(false)
  const [total, setTotal] = useState(0)

  const fetchPackages = async (searchFilters: SearchFilters) => {
    setLoading(true)
    try {
      const params = new URLSearchParams()
      if (searchFilters.query) params.append("q", searchFilters.query)
      if (searchFilters.category) params.append("category", searchFilters.category)
      if (searchFilters.license) params.append("license", searchFilters.license)
      if (searchFilters.sort) params.append("sort", searchFilters.sort)

      const response = await fetch(`/api/packages?${params}`)
      const data = await response.json()

      setPackages(data.packages || [])
      setTotal(data.total || 0)
    } catch (error) {
      console.error("Failed to fetch packages:", error)
    } finally {
      setLoading(false)
    }
  }

  useEffect(() => {
    fetchPackages(filters)
  }, [filters.category, filters.license, filters.sort])

  const handleSearch = (searchFilters: SearchFilters) => {
    setFilters((prev) => ({ ...prev, ...searchFilters }))
    if (searchFilters.query !== undefined) {
      fetchPackages({ ...filters, ...searchFilters })
    }
  }

  const activeFiltersCount =
    (filters.category ? 1 : 0) +
    (filters.license ? 1 : 0) +
    (filters.sort && filters.sort !== "relevance" ? 1 : 0)

  return (
    <div className="container py-8">
      <div className="space-y-8">
        <div className="space-y-4">
          <h1 className="text-4xl font-bold">Browse Packages</h1>
          <p className="text-muted-foreground">
            Explore {total} packages available in the RADS ecosystem.
          </p>
        </div>

        <div className="flex flex-col md:flex-row gap-4">
          <div className="flex-1">
            <SearchBar onSearch={handleSearch} disabled={loading} />
          </div>

          <Button
            variant="outline"
            onClick={() => setShowFilters(!showFilters)}
            className="md:w-auto"
          >
            <SlidersHorizontal className="mr-2 h-4 w-4" />
            Filters
            {activeFiltersCount > 0 && (
              <Badge variant="secondary" className="ml-2">
                {activeFiltersCount}
              </Badge>
            )}
          </Button>
        </div>

        {showFilters && (
          <div className="grid grid-cols-1 md:grid-cols-3 gap-4 rounded-lg border bg-card p-6">
            <div>
              <label className="text-sm font-medium mb-2 block">Category</label>
              <div className="flex flex-wrap gap-2">
                {CATEGORIES.map((cat) => (
                  <Badge
                    key={cat.value}
                    variant={filters.category === cat.value ? "default" : "outline"}
                    className="cursor-pointer hover:bg-secondary"
                    onClick={() => {
                      setFilters((prev) => ({
                        ...prev,
                        category: prev.category === cat.value ? undefined : cat.value,
                      }))
                    }}
                  >
                    {cat.label}
                  </Badge>
                ))}
              </div>
            </div>

            <div>
              <label className="text-sm font-medium mb-2 block">License</label>
              <div className="flex flex-wrap gap-2">
                {LICENSES.map((lic) => (
                  <Badge
                    key={lic.value}
                    variant={filters.license === lic.value ? "default" : "outline"}
                    className="cursor-pointer hover:bg-secondary"
                    onClick={() => {
                      setFilters((prev) => ({
                        ...prev,
                        license: prev.license === lic.value ? undefined : lic.value,
                      }))
                    }}
                  >
                    {lic.label}
                  </Badge>
                ))}
              </div>
            </div>

            <div>
              <label className="text-sm font-medium mb-2 block">Sort By</label>
              <div className="flex flex-wrap gap-2">
                {SORT_OPTIONS.map((opt) => (
                  <Badge
                    key={opt.value}
                    variant={filters.sort === opt.value ? "default" : "outline"}
                    className="cursor-pointer hover:bg-secondary"
                    onClick={() => {
                      setFilters((prev) => ({
                        ...prev,
                        sort: opt.value as any,
                      }))
                    }}
                  >
                    {opt.label}
                  </Badge>
                ))}
              </div>
            </div>
          </div>
        )}

        {(activeFiltersCount > 0 || filters.query) && (
          <div className="flex items-center gap-2 text-sm text-muted-foreground">
            <span>Active filters:</span>
            {filters.category && (
              <Badge variant="secondary" className="cursor-pointer" onClick={() => setFilters((prev) => ({ ...prev, category: undefined }))}>
                {CATEGORIES.find((c) => c.value === filters.category)?.label} ✕
              </Badge>
            )}
            {filters.license && (
              <Badge variant="secondary" className="cursor-pointer" onClick={() => setFilters((prev) => ({ ...prev, license: undefined }))}>
                {filters.license} ✕
              </Badge>
            )}
            {(filters.sort && filters.sort !== "relevance") && (
              <Badge variant="secondary" className="cursor-pointer" onClick={() => setFilters((prev) => ({ ...prev, sort: "relevance" }))}>
                {filters.sort} ✕
              </Badge>
            )}
            {filters.query && (
              <Badge variant="secondary" className="cursor-pointer" onClick={() => setFilters((prev) => ({ ...prev, query: "" }))}>
                "{filters.query}" ✕
              </Badge>
            )}
            <Button
              variant="ghost"
              size="sm"
              onClick={() => setFilters({})}
            >
              Clear all
            </Button>
          </div>
        )}

        {loading ? (
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
            {[...Array(6)].map((_, i) => (
              <Card key={i} className="h-64" />
            ))}
          </div>
        ) : packages.length === 0 ? (
          <div className="text-center py-16">
            <Package className="h-16 w-16 text-muted-foreground mx-auto mb-4" />
            <h3 className="text-xl font-semibold mb-2">No packages found</h3>
            <p className="text-muted-foreground">
              Try adjusting your search or filters to find what you're looking for.
            </p>
          </div>
        ) : (
          <>
            <div className="flex items-center justify-between text-sm text-muted-foreground mb-4">
              <span>Showing {packages.length} of {total} packages</span>
            </div>
            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
              {packages.map((pkg) => (
                <PackageCard key={pkg.name} package={pkg} />
              ))}
            </div>
          </>
        )}
      </div>
    </div>
  )
}

"use client"

import { useState } from "react"
import { Button } from "@/components/ui/button"
import { Input } from "@/components/ui/input"
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from "@/components/ui/card"
import { apiClient } from "@/lib/api"
import { Upload, CheckCircle } from "lucide-react"
import { useRouter } from "next/navigation"

const CATEGORIES = [
  "database",
  "web",
  "utils",
  "media",
  "networking",
  "testing",
  "cli",
  "other",
]

const LICENSES = ["MIT", "Apache-2.0", "GPL-3.0", "BSD-3", "ISC"]

export default function PublishPage() {
  const router = useRouter()
  const [loading, setLoading] = useState(false)
  const [success, setSuccess] = useState(false)
  const [error, setError] = useState<string | null>(null)
  const [formData, setFormData] = useState({
    name: "",
    displayName: "",
    description: "",
    version: "1.0.0",
    repository: "",
    homepage: "",
    category: "",
    license: "MIT",
    keywords: "",
    readme: "",
  })

  const user = JSON.parse(localStorage.getItem("github_user") || "{}")

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault()
    setLoading(true)
    setError(null)

    if (!user.login) {
      setError("You must be logged in to publish packages")
      router.push("/login")
      return
    }

    try {
      const token = localStorage.getItem("github_token") || ""

      const packageData = {
        name: formData.name.trim(),
        displayName: formData.displayName.trim() || formData.name.trim(),
        description: formData.description.trim(),
        version: formData.version,
        author: user.login,
        repository: formData.repository.trim(),
        homepage: formData.homepage.trim(),
        category: formData.category,
        license: formData.license,
        keywords: formData.keywords
          .split(",")
          .map((k) => k.trim())
          .filter(Boolean),
        readme: formData.readme,
      }

      const response = await apiClient.publishPackage(packageData, token)

      if (response.success) {
        setSuccess(true)
        setTimeout(() => router.push(`/packages/${formData.name}`), 2000)
      } else {
        setError("Failed to publish package")
      }
    } catch (err) {
      setError((err as Error).message)
    } finally {
      setLoading(false)
    }
  }

  if (success) {
    return (
      <div className="container flex items-center justify-center min-h-[calc(100vh-8rem)]">
        <Card className="w-full max-w-md text-center">
          <CardContent className="py-12">
            <CheckCircle className="h-16 w-16 text-green-500 mx-auto mb-4" />
            <CardTitle className="mb-2">Package Published!</CardTitle>
            <CardDescription>
              Your package has been successfully published to the RADS registry.
            </CardDescription>
            <Button
              onClick={() => router.push("/packages")}
              className="mt-6"
            >
              Browse Packages
            </Button>
          </CardContent>
        </Card>
      </div>
    )
  }

  return (
    <div className="container py-8">
      <div className="max-w-2xl mx-auto">
        <div className="mb-8">
          <h1 className="text-4xl font-bold mb-2">Publish Package</h1>
          <p className="text-muted-foreground">
            Share your RADS package with the community.
          </p>
        </div>

        {error && (
          <div className="rounded-md bg-destructive/10 p-4 text-destructive mb-6">
            <p className="font-medium">Error</p>
            <p className="text-sm mt-1">{error}</p>
          </div>
        )}

        <Card>
          <CardHeader>
            <CardTitle>Package Information</CardTitle>
            <CardDescription>
              Provide details about your RADS package.
            </CardDescription>
          </CardHeader>

          <CardContent>
            <form onSubmit={handleSubmit} className="space-y-6">
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div className="space-y-2">
                  <label className="text-sm font-medium">
                    Package Name <span className="text-destructive">*</span>
                  </label>
                  <Input
                    required
                    placeholder="my-awesome-package"
                    value={formData.name}
                    onChange={(e) =>
                      setFormData({ ...formData, name: e.target.value })
                    }
                    pattern="[a-z0-9-]+"
                    title="Lowercase letters, numbers, and hyphens only"
                  />
                </div>

                <div className="space-y-2">
                  <label className="text-sm font-medium">
                    Display Name
                  </label>
                  <Input
                    placeholder="My Awesome Package"
                    value={formData.displayName}
                    onChange={(e) =>
                      setFormData({ ...formData, displayName: e.target.value })
                    }
                  />
                </div>
              </div>

              <div className="space-y-2">
                <label className="text-sm font-medium">
                  Description <span className="text-destructive">*</span>
                </label>
                <textarea
                  required
                  className="flex min-h-[100px] w-full rounded-md border border-input bg-background px-3 py-2 text-sm ring-offset-background placeholder:text-muted-foreground focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2"
                  placeholder="A brief description of your package..."
                  value={formData.description}
                  onChange={(e) =>
                    setFormData({ ...formData, description: e.target.value })
                  }
                />
              </div>

              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div className="space-y-2">
                  <label className="text-sm font-medium">
                    Version <span className="text-destructive">*</span>
                  </label>
                  <Input
                    required
                    placeholder="1.0.0"
                    value={formData.version}
                    onChange={(e) =>
                      setFormData({ ...formData, version: e.target.value })
                    }
                  />
                </div>

                <div className="space-y-2">
                  <label className="text-sm font-medium">
                    Repository URL <span className="text-destructive">*</span>
                  </label>
                  <Input
                    required
                    type="url"
                    placeholder="https://github.com/user/repo"
                    value={formData.repository}
                    onChange={(e) =>
                      setFormData({ ...formData, repository: e.target.value })
                    }
                  />
                </div>
              </div>

              <div className="space-y-2">
                <label className="text-sm font-medium">Homepage URL</label>
                <Input
                  type="url"
                  placeholder="https://package-site.com"
                  value={formData.homepage}
                  onChange={(e) =>
                    setFormData({ ...formData, homepage: e.target.value })
                  }
                />
              </div>

              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div className="space-y-2">
                  <label className="text-sm font-medium">Category</label>
                  <select
                    className="flex h-10 w-full rounded-md border border-input bg-background px-3 py-2 text-sm ring-offset-background focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2"
                    value={formData.category}
                    onChange={(e) =>
                      setFormData({ ...formData, category: e.target.value })
                    }
                  >
                    <option value="">Select category...</option>
                    {CATEGORIES.map((cat) => (
                      <option key={cat} value={cat}>
                        {cat.charAt(0).toUpperCase() + cat.slice(1)}
                      </option>
                    ))}
                  </select>
                </div>

                <div className="space-y-2">
                  <label className="text-sm font-medium">License</label>
                  <select
                    className="flex h-10 w-full rounded-md border border-input bg-background px-3 py-2 text-sm ring-offset-background focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2"
                    value={formData.license}
                    onChange={(e) =>
                      setFormData({ ...formData, license: e.target.value })
                    }
                  >
                    {LICENSES.map((lic) => (
                      <option key={lic} value={lic}>
                        {lic}
                      </option>
                    ))}
                  </select>
                </div>
              </div>

              <div className="space-y-2">
                <label className="text-sm font-medium">Keywords</label>
                <Input
                  placeholder="database, sql, query"
                  value={formData.keywords}
                  onChange={(e) =>
                    setFormData({ ...formData, keywords: e.target.value })
                  }
                />
                <p className="text-xs text-muted-foreground">
                  Comma-separated keywords for better discoverability
                </p>
              </div>

              <div className="space-y-2">
                <label className="text-sm font-medium">
                  README <span className="text-destructive">*</span>
                </label>
                <textarea
                  required
                  className="flex min-h-[200px] w-full rounded-md border border-input bg-background px-3 py-2 text-sm ring-offset-background placeholder:text-muted-foreground focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-offset-2 font-mono"
                  placeholder="# My Package

## Installation

\`\`\`bash
rstar install my-package
\`\`\`

## Usage

\`\`\`rads
import my_package

blast main() {
    echo("Hello from my package!")
}
\`\`\`"
                  value={formData.readme}
                  onChange={(e) =>
                    setFormData({ ...formData, readme: e.target.value })
                  }
                />
              </div>

              <Button type="submit" disabled={loading} size="lg" className="w-full">
                {loading ? (
                  <>
                    <div className="inline-block animate-spin rounded-full h-4 w-4 border-2 border-primary mr-2" />
                    Publishing...
                  </>
                ) : (
                  <>
                    <Upload className="mr-2 h-5 w-5" />
                    Publish Package
                  </>
                )}
              </Button>
            </form>
          </CardContent>
        </Card>
      </div>
    </div>
  )
}

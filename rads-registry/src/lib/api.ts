const API_BASE = process.env.NEXT_PUBLIC_API_BASE || "https://api.registry.rads-lang.org"

interface ApiClientOptions {
  method?: "GET" | "POST"
  body?: any
  headers?: Record<string, string>
}

export class ApiClient {
  private baseUrl: string

  constructor(baseUrl: string = API_BASE) {
    this.baseUrl = baseUrl
  }

  async dispatch<T>(eventType: string, payload?: any): Promise<T> {
    const token = localStorage.getItem("github_token")

    try {
      const response = await fetch(`${this.baseUrl}/dispatch`, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          Accept: "application/json",
          ...(token && { Authorization: `Bearer ${token}` }),
          ...this.getHeaders(),
        },
        body: JSON.stringify({
          event_type: eventType,
          client_payload: payload,
        }),
      })

      if (!response.ok) {
        const error = await response.json()
        throw new Error(error.error || "API request failed")
      }

      const output = await response.json()
      return output
    } catch (error) {
      console.error("API request failed:", error)
      throw error
    }
  }

  private getHeaders(): Record<string, string> {
    return {}
  }

  async getPackages(filters: {
    query?: string
    category?: string
    license?: string
    sort?: string
  }): Promise<{
    packages: any[]
    total: number
    page: number
    pageSize: number
  }> {
    return this.dispatch("api-list-packages", filters)
  }

  async searchPackages(filters: {
    query?: string
    category?: string
    license?: string
    sort?: string
  }): Promise<{
    packages: any[]
    total: number
  }> {
    return this.dispatch("api-search", filters)
  }

  async getPackage(name: string): Promise<any> {
    const packages = await this.getPackages({})
    return packages.packages.find((pkg: any) => pkg.name === name)
  }

  async publishPackage(
    packageData: any,
    token: string
  ): Promise<{ success: boolean; data: any }> {
    return this.dispatch("api-publish", {
      package: packageData,
      token,
    })
  }

  async oauthCallback(code: string, state: string): Promise<{
    success: boolean
    data?: { token: string; user: any }
    error?: string
  }> {
    return this.dispatch("api-auth-callback", { code, state })
  }
}

export const apiClient = new ApiClient()

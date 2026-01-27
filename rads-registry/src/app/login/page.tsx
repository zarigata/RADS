"use client"

import { useState, useEffect } from "react"
import { Button } from "@/components/ui/button"
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from "@/components/ui/card"
import { GitHub } from "lucide-react"
import { apiClient } from "@/lib/api"

export default function LoginPage() {
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    const urlParams = new URLSearchParams(window.location.search)
    const code = urlParams.get("code")
    const state = urlParams.get("state")

    if (code && state) {
      handleOAuthCallback(code, state)
    }
  }, [])

  const handleOAuthCallback = async (code: string, state: string) => {
    setLoading(true)
    setError(null)

    try {
      const response = await apiClient.oauthCallback(code, state)

      if (response.success && response.data) {
        localStorage.setItem("github_token", response.data.token)
        localStorage.setItem("github_user", JSON.stringify(response.data.user))

        window.location.href = "/publish"
      } else if (response.error) {
        setError(response.error)
      } else {
        setError("Authentication failed")
      }
    } catch (err) {
      setError((err as Error).message)
    } finally {
      setLoading(false)
    }
  }

  const initiateOAuth = () => {
    const clientId = process.env.NEXT_PUBLIC_GITHUB_CLIENT_ID || ""
    const state = Math.random().toString(36).substring(2, 15)
    const redirectUri = `${window.location.origin}/api/auth/callback`
    const scope = "read:user,read:org,repo"

    const authUrl = `https://github.com/login/oauth/authorize?client_id=${clientId}&redirect_uri=${encodeURIComponent(
      redirectUri
    )}&scope=${encodeURIComponent(scope)}&state=${state}`

    window.location.href = authUrl
  }

  return (
    <div className="container flex items-center justify-center min-h-[calc(100vh-8rem)]">
      <Card className="w-full max-w-md">
        <CardHeader className="text-center">
          <GitHub className="h-12 w-12 text-primary mx-auto mb-4" />
          <CardTitle>Login with GitHub</CardTitle>
          <CardDescription>
            Sign in to publish packages and manage your account
          </CardDescription>
        </CardHeader>

        <CardContent className="space-y-4">
          {loading ? (
            <div className="text-center py-8">
              <div className="inline-block animate-spin rounded-full h-8 w-8 border-b-2 border-primary" />
              <p className="mt-4 text-sm text-muted-foreground">
                Authenticating with GitHub...
              </p>
            </div>
          ) : error ? (
            <div className="space-y-4">
              <div className="rounded-md bg-destructive/10 p-4 text-destructive">
                <p className="font-medium">Authentication Failed</p>
                <p className="text-sm mt-1">{error}</p>
              </div>
              <Button onClick={initiateOAuth} className="w-full">
                <GitHub className="mr-2 h-4 w-4" />
                Try Again
              </Button>
            </div>
          ) : (
            <div className="space-y-4">
              <div className="rounded-md bg-muted p-4">
                <p className="text-sm">
                  By logging in, you agree to our{" "}
                  <a href="/terms" className="text-primary hover:underline">
                    Terms of Service
                  </a>{" "}
                  and{" "}
                  <a href="/privacy" className="text-primary hover:underline">
                    Privacy Policy
                  </a>
                </p>
              </div>
              <Button onClick={initiateOAuth} className="w-full">
                <GitHub className="mr-2 h-4 w-4" />
                Sign in with GitHub
              </Button>
            </div>
          )}
        </CardContent>
      </Card>
    </div>
  )
}

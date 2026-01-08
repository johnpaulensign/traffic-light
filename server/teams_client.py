"""Microsoft Graph client for Teams presence."""

import os
from typing import Optional
import requests
from msal import PublicClientApplication, SerializableTokenCache

GRAPH_PRESENCE_ENDPOINT = "https://graph.microsoft.com/v1.0/me/presence"
SCOPE = ["Presence.Read"]


class TeamsClient:
    """Client for fetching Teams presence via Microsoft Graph."""

    def __init__(self, client_id: str, tenant_id: str, cache_path: Optional[str] = None):
        self.client_id = client_id
        self.tenant_id = tenant_id
        self.authority = f"https://login.microsoftonline.com/{tenant_id}"
        self.cache_path = cache_path or os.path.join(
            os.path.dirname(__file__), 'token_cache.bin'
        )
        self._cache: Optional[SerializableTokenCache] = None
        self._app: Optional[PublicClientApplication] = None

    def _get_cache(self) -> SerializableTokenCache:
        if self._cache is None:
            self._cache = SerializableTokenCache()
            if os.path.exists(self.cache_path):
                with open(self.cache_path, 'rb') as f:
                    self._cache.deserialize(f.read().decode())
        return self._cache

    def _save_cache(self) -> None:
        if self._cache and self._cache.has_state_changed:
            with open(self.cache_path, 'wb') as f:
                f.write(self._cache.serialize().encode())

    def _get_app(self) -> PublicClientApplication:
        if self._app is None:
            self._app = PublicClientApplication(
                self.client_id,
                authority=self.authority,
                token_cache=self._get_cache()
            )
        return self._app

    def get_access_token(self) -> str:
        """Get an access token, prompting for device flow if needed."""
        app = self._get_app()
        accounts = app.get_accounts()

        if accounts:
            result = app.acquire_token_silent(SCOPE, account=accounts[0])
            if result and 'access_token' in result:
                self._save_cache()
                return result['access_token']

        # Device flow
        flow = app.initiate_device_flow(scopes=SCOPE)
        if 'user_code' not in flow:
            raise Exception(f"Failed to create device flow: {flow}")

        print(f"To authenticate, visit {flow['verification_uri']} and enter code: {flow['user_code']}")
        result = app.acquire_token_by_device_flow(flow)

        if 'access_token' in result:
            self._save_cache()
            return result['access_token']
        else:
            raise Exception(f"Could not obtain access token: {result}")

    def get_presence(self) -> str:
        """Fetch current Teams presence availability."""
        token = self.get_access_token()
        headers = {'Authorization': f'Bearer {token}'}
        resp = requests.get(GRAPH_PRESENCE_ENDPOINT, headers=headers)
        resp.raise_for_status()
        return resp.json()['availability']

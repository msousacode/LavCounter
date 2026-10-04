#pragma once
// Copy to CloudPrivate.h (ignored by Git). Keep empty until real access is supplied.
// clientId is generated on the device, never configured here.
#define LAVCOUNTER_CLOUD_CONFIGURE(c) do { \
 (c).enabled=false; \
 (c).ssid=""; (c).wifiPassword=""; \
 (c).projectId=""; (c).databaseId="(default)"; \
 (c).signupUrl=""; (c).premiumUrl=""; (c).caPem=""; \
} while(0)
// Optional: LAVCOUNTER_CLOUD_PROVIDERS(authProvider, premiumProvider)
// Include provider headers here and assign long-lived provider pointers.
// Never put a service-account private key or the SaaS user's password here.
// Temporary Firebase integration: see docs/NUVEM.md. Credentials go in the
// separately ignored CloudCredentials.h, provisioned once by prepare-firebase-test.ps1.

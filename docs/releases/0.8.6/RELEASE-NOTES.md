# OpenUp 0.8.6 (20 October 2026)

- **OpenBrowser Lite 0.4.1** is in the standard set, in SYS:Utilities/OpenBrowserLite. It's a small web
  browser (GadTools, no WebKit, about 100 KB) that lays out pages with weblayout and fetches https:// through
  OpenTLS. It's taken from the openamigabrowser v0.4.1 release and pinned by its sha256.
- **TLS 1.3.** OpenTLS 1.3 adds a TLS 1.3 client, with TLS 1.2 still on the same connection, and session
  tickets. OpenCrypto 2.1 goes with it, and the CA bundle is refreshed.
- **OpenUp 0.8.6.** OpenUp's own tools (OpenUpMenu, Install-OpenUp, Uninstall-OpenUp) are at the new version.
  ZuneMaster and the OpenGPU SDK's SDL library were rebuilt from the same sources.

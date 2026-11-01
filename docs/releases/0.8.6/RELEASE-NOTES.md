# GFOpenUp 0.8.6 (20 October 2026)

- **OpenBrowser Lite 0.4.1** is in the standard set, in SYS:Utilities/OpenBrowserLite. It's a small web
  browser (GadTools, no WebKit, about 100 KB) that lays out pages with weblayout and fetches https:// through
  GFOpenTLS. It's taken from the openamigabrowser v0.4.1 release and pinned by its sha256.
- **TLS 1.3.** GFOpenTLS 1.3 adds a TLS 1.3 client, with TLS 1.2 still on the same connection, and session
  tickets. GFOpenCrypto 2.1 goes with it, and the CA bundle is refreshed.
- **GFOpenUp 0.8.6.** GFOpenUp's own tools (GFOpenUpMenu, Install-GFOpenUp, Uninstall-GFOpenUp) are at the new version.
  ZuneMaster and the GFOpenGPU SDK's SDL library were rebuilt from the same sources.

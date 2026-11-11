import UIKit
import WebKit

/// Hosts the game (the bundled `www` folder) in a full-screen web view. Everything is loaded from
/// the app bundle, so the game needs no network connection.
final class GameViewController: UIViewController, WKUIDelegate, WKNavigationDelegate, WKScriptMessageHandler {
    private var webView: WKWebView!

    override var prefersStatusBarHidden: Bool { true }
    override var prefersHomeIndicatorAutoHidden: Bool { true }
    override var preferredScreenEdgesDeferringSystemGestures: UIRectEdge { .all }

    override func loadView() {
        let config = WKWebViewConfiguration()
        config.allowsInlineMediaPlayback = true
        config.mediaTypesRequiringUserActionForPlayback = []

        if selfTest {
            // Self-test (used by the automated simulator run): report page errors to the app log.
            let js = "window.addEventListener('error',function(e){window.webkit.messageHandlers.selftest.postMessage('JS ERROR '+(e.message||'resource failed: '+((e.target&&(e.target.src||e.target.href))||'?')))},true);"
            config.userContentController.addUserScript(WKUserScript(source: js, injectionTime: .atDocumentStart, forMainFrameOnly: true))
            config.userContentController.add(self, name: "selftest")
        }

        webView = WKWebView(frame: .zero, configuration: config)
        webView.uiDelegate = self
        webView.navigationDelegate = self
        webView.isOpaque = false
        webView.backgroundColor = UIColor(red: 0.043, green: 0.09, blue: 0.063, alpha: 1)
        webView.scrollView.isScrollEnabled = false
        webView.scrollView.bounces = false
        webView.scrollView.contentInsetAdjustmentBehavior = .never
        webView.allowsLinkPreview = false
        webView.allowsBackForwardNavigationGestures = false
        view = webView
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        guard let www = Bundle.main.url(forResource: "www", withExtension: nil) else {
            fatalError("The www folder is missing from the app bundle. Run tools/build_pages.py.")
        }
        webView.loadFileURL(www.appendingPathComponent("index.html"), allowingReadAccessTo: www)
    }

    // The game asks for motion access when the player turns on motion look. Grant it for the
    // app's own bundled page so there is no second prompt.
    func webView(_ webView: WKWebView,
                 requestDeviceOrientationAndMotionPermissionFor origin: WKSecurityOrigin,
                 initiatedByFrame frame: WKFrameInfo,
                 decisionHandler: @escaping (WKPermissionDecision) -> Void) {
        decisionHandler(.grant)
    }

    // Stay inside the bundled game: links to other sites open in Safari.
    func webView(_ webView: WKWebView,
                 decidePolicyFor navigationAction: WKNavigationAction,
                 decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
        if let url = navigationAction.request.url, !url.isFileURL, url.scheme != "about" {
            UIApplication.shared.open(url)
            decisionHandler(.cancel)
            return
        }
        decisionHandler(.allow)
    }

    // MARK: Self-test. Launch with "-selftest YES" to start a game automatically and log the result.

    private var selfTest: Bool { UserDefaults.standard.bool(forKey: "selftest") }

    func userContentController(_ userContentController: WKUserContentController, didReceive message: WKScriptMessage) {
        NSLog("SELFTEST %@", String(describing: message.body))
    }

    func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
        guard selfTest else { return }
        DispatchQueue.main.asyncAfter(deadline: .now() + 6) {
            webView.evaluateJavaScript("document.getElementById('startBtn').click()", completionHandler: nil)
        }
        DispatchQueue.main.asyncAfter(deadline: .now() + 20) {
            let probe = "JSON.stringify({three:!!window.THREE,cannon:!!window.CANNON,canvas:!!document.querySelector('canvas'),webgl:!!(document.querySelector('canvas')&&document.querySelector('canvas').width>0&&document.createElement('canvas').getContext('webgl')),playing:!document.getElementById('play').hidden})"
            webView.evaluateJavaScript(probe) { result, error in
                NSLog("SELFTEST RESULT %@", String(describing: result ?? error?.localizedDescription ?? "none"))
            }
        }
    }

    // If iOS stops the web process (low memory), reload the game.
    func webViewWebContentProcessDidTerminate(_ webView: WKWebView) {
        webView.reload()
    }
}

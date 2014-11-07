(defproject ifeel-funny "0.1.0-SNAPSHOT"
  :description "We are going to try to send data to an iFeel Mouse."
  :url "http://example.com/FIXME"
  :license {:name "Eclipse Public License"
            :url "http://www.eclipse.org/legal/epl-v10.html"}
  :dependencies [[org.clojure/clojure "1.6.0"]
                 [org.usb4java/usb4java "1.2.0"]]
  :main ^:skip-aot ifeel-funny.core
  :target-path "target/%s"
  :profiles {:uberjar {:aot :all}})

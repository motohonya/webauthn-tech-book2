= コラム - WebAuthnライブラリ、Webアプリケーションフレームワーク、アプリケーション。それぞれの責務

WebAuthnもこの3月にW3C勧告となり、各ブラウザでのWebAuthnのサポートも進んできました。
OSSでも、WebAuthn認証を実現するためのライブラリが各言語毎に様々リリースされており、
いよいよWebAuthnを認証で採用する環境が整いつつあるといえるでしょう。

しかしながら、現実にWebAuthnを採用しようとすると、ライブラリを活用してもアプリ側に残された実装量の多さ、
複雑さに驚くことになるかもしれません。WebAuthnによる認証に取り組むにあたっては、そのユーザー登録・ユーザー認証プロセスで
どのような処理が行われるのか、その内、WebAuthnのライブラリはどこまでをカバーするのか、理解しておくことが大切です。
本章では、WebAuthn4JというJava製のWebAuthnライブラリの作者である筆者が、WebAuthn4JをはじめとしたWebAuthnのライブラリと、
認証フレームワーク、そしてアプリケーションそれぞれの所掌範囲について整理したいと思います。

== WebAuthnのユーザー登録・認証フロー

まずはWebAuthnのユーザー登録・認証フローをおさらいしましょう。ここでは概要レベルの説明にとどめますので、
より詳しい説明が必要な方はYahooの合路さんによる「FIDO認証によるパスワードレスログイン実装入門」
（ https://www.slideshare.net/techblogyahoo/fido-124019677 ）や、DUO SecurityによるWebAuthn.guide
(https://webauthn.guide/) など、より詳細に説明した資料を参照して下さい。

=== ユーザー登録

//image[registration-flow][ユーザー登録フロー]

WebAuthnのユーザー登録は、簡単に言うと、ユーザーに紐づけるCredential（公開鍵）をAuthenticatorで生成し、
生成したCredentialをサーバーでユーザーに紐づけて保存する処理です。WebAuthnを使用するサイト（Relying Party）は
AuthenticatorにCredential（公開鍵）の生成を要求するにあたって、ブラウザ（Client Platform）のWebAuthn JS APIを呼び出します。
この呼び出しに際して、WebAuthnのセキュリティを実現するためのパラメータが色々必要です。このパラメータは、
ブラウザ上で動作するサイトのフロントエンドアプリケーションだけで決定できるものだけでなく、ChallengeやUserHandleなど、
サイトのサーバーサイドアプリケーション側で決定すべき値もあります。そのため、WebAuthnのユーザー登録フローは、

# サーバーサイドでWebAuthnのパラメータ値の生成、フロントエンドへの引き渡し
# フロントエンドでWebAuthn JS Credential（公開鍵）生成APIの呼出
# 生成されたCredential（公開鍵）をサーバーサイドに送信
# サーバーサイドでCredential（公開鍵）を検証
# 問題がなければユーザーに紐づけて登録

という流れになります。

=== ユーザー認証

//image[authentication-flow][ユーザー認証フロー]

WebAuthnのユーザー認証も同様で、Credential（公開鍵と対になる秘密鍵から生成した署名）をAuthenticatorから取得し、
取得した署名をサーバーでユーザーに紐づけて保存されている公開鍵で検証した可否で認証を行う処理です。
WebAuthnを使用するサイト（Relying Party）はAuthenticatorにCredential（署名）の取得を要求するにあたって、
ブラウザ（Client Platform）のWebAuthn JS APIを呼び出します。この呼び出しに際して、WebAuthnのセキュリティを実現するための
パラメータが色々必要であり、WebAuthnのユーザー検証フローは、

# サーバーサイドでWebAuthnのパラメータ値の生成、フロントエンド側への引き渡し
# フロントエンドアプリケーションでWebAuthn JS Credential（署名）取得APIの呼び出し
# 取得されたCredential（署名）をサーバーサイドに送信
# サーバーサイドでCredential（署名）を検証
# 問題がなければユーザー認証成功
# ログインカウンタの更新

という流れになります。

== WebAuthnライブラリの所掌範囲

さて、このWebAuthnのフローの中で、WebAuthnライブラリのサポートする範囲はどこでしょうか。
実は、多くのライブラリで、ユーザー登録・認証のフローのどちらであっても、「4. サーバーサイドでCredentialを検証」の
部分だけです。拙作のWebAuthn4Jだけでなく、Yubico製のjava-webauthn-serverや、.NET向けのFIDO2 .NET Libraryなど、
他の主要なWebAuthnライブラリでも概ね同様です。

WebAuthnのパラメータ値を生成し、フロントエンド側に引き渡すためのREST APIエンドポイントの提供は多くのWebAuthnライブラリで
行われません。これは何故かというと、REST APIエンドポイントを提供するために特定のWebアプリケーションフレームワークに
依存してしまうと、そのWebアプリケーションフレームワークへの依存関係が生まれてしまい、汎用性が失われるためです。
Challenge管理も同様で、Challengeをセッションに紐づけるためにWebアプリケーションフレームワークのAPIを使用してしまうと、
そのWebアプリケーションフレームワークへの依存関係が生まれてしまいます。

== Webアプリの責務

こういった、WebAuthn認証に必要だが汎用性を重視したWebAuthnライブラリでは提供されていない機能は、基本的には、
Webアプリケーション自身で実装して埋めていかなければなりません。WebAuthn4Jの場合、汎用性の為に省略している機能は、
「オプションパラメータを提供するREST APIエンドポイントの提供」「登録エンドポイントの提供」「認証エンドポイントの提供」
「Challenge管理」「ユーザー情報、Credentialの永続化」が挙げられます。

しかしながら、それぞれ実装量の多い処理であり、また、これらの処理の多くは特定のWebアプリケーションフレームワークに
依存しているとはいえ、アプリ毎の要件の違いの大きい「ユーザー情報、Credentialの永続化」処理を除き、
アプリケーション固有ではありません。Webアプリケーションフレームワーク側に取り込んだり、
Webアプリケーションフレームワークのサポートライブラリ化する余地があります。

== Webアプリケーションフレームワークの所掌範囲

前述のように、WebAuthnによる認証を提供するのに必要だが汎用性の為に省略している機能の内、
「オプションパラメータを提供するREST APIエンドポイントの提供」「登録エンドポイントの提供」「認証エンドポイントの提供」
「Challenge管理」は、Webアプリケーションフレームワーク側に取り込んだり、Webアプリケーションフレームワークの
サポートライブラリ化する余地があります。

WebAuthn4Jの場合、Springアプリケーションで活用する場合の為に、WebAuthn4JをSpring Security向けにラップした
Spring Security WebAuthnというライブラリを準備しています。

Spring Security WebAuthnでは、Spring Securityに依存関係を持ち、Spring Securityの認証フレームワークとしての機能を
フルに活用してWebAuthn4Jが省略した「オプションパラメータを提供するREST APIエンドポイントの提供」
「登録エンドポイントの提供」「認証エンドポイントの提供」「Challenge管理」機能を提供しています。
WebAuthn4Jを直接アプリケーションから使用するのではなく、Spring Security WebAuthnを使用することで、
これらの機能を再実装する必要はありません。WebAuthn4Jに対するSpring Security WebAuthnのように、
Webアプリケーション特化のWebAuthnライブラリが提供されている場合は、アプリケーションは、本来のビジネスロジックの実装に
注力することが出来ます。

一方、「ユーザー情報、Credentialの永続化」については、前述の通り、ユーザー情報は、Credentialだけでなく、
ユーザーの姓名や性別など、他の情報もまとめて保持したり、アプリ毎の要件の違いが大きいので、アプリ側の責務としています。
これは、Spring Security WebAuthnのベースとなっている認証フレームワークであるSpring Securityの設計の踏襲でもあります。

== まとめ

以上のように、WebAuthn4Jをはじめとした世の中のWebAuthnライブラリはフレームワークへの依存関係を排し、汎用性を担保する為に、
Assertion/Attestaionの検証に機能を絞っている場合が多くあります。その場合、アプリケーション側での実装負担が
大きくなりがちです。WebAuthnの実装にあたっては、使用を検討しているライブラリがどこまで機能を提供するのか、
Webアプリケーションフレームワークに特化した機能も提供してくれるのか確認すると良いでしょう。
もし、フレームワーク中立な実装になっている場合は、「オプションパラメータを提供するREST APIエンドポイントの提供」
「登録エンドポイントの提供」「認証エンドポイントの提供」「Challenge管理」の実装を独自に行う必要があることを念頭に
工数の見積もりを行う必要があることに注意すると良いでしょう。そして、WebAuthnライブラリとWebアプリケーションフレームワークの
結合が上手く実装できた暁には、汎用化したサポートライブラリとしてまとめ、OSSとして公開すると、後続の人達が苦労せずに済み、
また、メンテを行う労力も分担出来る、かもしれません。


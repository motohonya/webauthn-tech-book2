= FIDOとOpenID Connectの関係性について

こんにちは@super_readerです。普段は認証やID連携のことをやっており、その中でも主にFIDO(特にWebAuthn)を扱っているWebエンジニアです。

今回FIDOに関してなにかネタはないかと考えたときに、
自分が取り組んでいる認証とID連携の仕組みについて関連付けたものを書いたら、
読んでくださる方の役にたつのでは思いました。

そこで、今回は「FIDOとOpenID Connectの関係性」について書かせていただきました。
私自身、ID連携についての知識をかじった後にFIDOなどの認証の世界を学んでいった経緯もあり、
それぞれの仕様について触れてきた立場から、
簡単にはなりますがそれぞれの仕様がどのように関連しているのかを解説したいと思います。

今回解説したい題材はこちらになります。

 * OpenID ConnectにとってのFIDOとは？
 * FIDOとOpenID ConnectでのRelying Partyについて
 * FIDOとSelf-issuedについて

それではさっそくFIDOとOpenID Connectの関係を見てみましょう。

== OpenID ConnectにとってのFIDOとは？
まずはOpenID Connectについて簡単に説明したいと思います。
@<br>{}OpenID ConnectはID連携の仕様の一つです。
ID連携とは連携先のサービス(Relying Party)に対して、IDやパスワードを渡すことなく、
ユーザーの認証情報を提供して、OpenId Providerに保存されているユーザーデータ（属性情報）にアクセスしたり、
SingleSingOnを実現することが可能になる仕組みを指します。
ID連携の仕様の一つであるOpenID Connectはユーザー認証情報を含む改ざん検知用の署名付きID Tokenを発行し、
ユーザーのセッション管理を行ったり、認可用のトークンの発行などに関する仕様です。
現在ではさまざまなRelying Partyが存在しており、OpenID Providerと連携をしながらサービスを展開しています。
また、OpenId Providerは認証と認可の機能、そして、サービスがほしい属性情報を持っているサービスになります。
実際の企業でいいますとGoogleやYahoo! JAPAN、LINEなどのID Providerを指します。

簡単にOpenID Connectに関して説明をさせていただきましたが、細かな仕様の説明は割愛させていただきます。

その代わりにOpenID Connectの理解に役立ちそうなサイトを記載させていただきます。

 * @<href>{https://qiita.com/TakahikoKawasaki/items/498ca08bbfcc341691fe,一番分かりやすい OpenID Connect の説明}
 * @<href>{https://www.slideshare.net/kura_lab/openid-connect-id,OpenID Connnect入門}
 * @<href>{https://qiita.com/TakahikoKawasaki/items/8f0e422c7edd2d220e06,IDトークンが分かれば OpenID Connect が分かる}

=== OpenID ConnectとFIDOのフローを確認してみよう
さて、本題に入りたいと思います。
OpenID ConnectとFIDOの関係を考えていく上で、まずはOpenID Connectがどのような流れで処理をされているのかを見てみましょう。
@<img>{oidc}はOpenID Connectのフローの一つであるAuthorization Codeフローを簡単に書いたものです。

//image[oidc][Authorization Codeフロー]

@<img>{oidc}はID連携を行いたいサービス(Relying Party)が、OpenId Providerの行った認証結果を得られる
トークンであるID Tokenを発行し、それをもとに安全に属性情報をやり取りするためのフローになります。

@<img>{oidc}でも書かれていますがRelying Partyに認可コードを渡すための「ユーザー認証」を行っています。
OpenID Connectの使用説明の中にOPTIONですが認証方法(Authentication Methods References)について以下のような記述があります。

//lead{
認証時に用いられた認証方式を示す識別子文字列の JSON 配列. 例として, パスワードと OTP 認証が両方行われたことを示すといったケースが考えられる.
@<b>{amr Claim にどのような値を用いるかは本仕様の定めるところではない.}
この値の意味するところはコンテキストによって異なる可能性があるため, この Claim を利用する場合は, 関係者間で値の意味するところについて合意しておくこと.
amr は大文字小文字を区別する文字列である.
//}

このように、OpenID Connectの仕様の中でも認証方法については仕様で細かく言及されていません。
これは言い換えると、認証方法に関してはOpenId Providerの実装方法に委ねられているということが言えます。

さて、ここでFIDO(WebAuthn)の認証フローを見てみましょう。

//image[fido][WebAuthnの認証フロー]

わざとらしく書いてますが、@<img>{fido}を見てみるとFIDOの仕様は認証だけで閉じています。
@<br>{}ここで注目してほしいことは、FIDOは認証の仕様ということです。
そして、OpenID ConnectのAuthorization Codeフローでも書かれていた「ユーザー認証」の部分にFIDOをそっくりそのまま入れ込むことができます。
これは、OPがFIDO対応の認証手段を持っていた場合にOpenID Connectのフローの中にFIDOを組み込むことが可能ということを示しています。
OpenID ConnectのAuthorization CodeフローにFIDO(WebAuthn)のフローを追加したものが@<img>{fido_in_oidc}です。

//image[fido_in_oidc][Authorization CodeフローにFIDOのフローを追加した図][scale=1.0]

このように、ID連携の仕様であるOpenID Connectと認証の仕組みであるFIDOは同居することが可能であり、組み合わせることでID連携をしたいサービスでもFIDOを使った認証体験を実現することが可能となります。

=== 実際の具体例を見てみよう
この仕組みが実際に行われているのがYahoo!ID連携です。
Yahoo!ID連携の画面フローをもとにどのような処理が行われているのかをたどっていきましょう。

今回は具体例としてヤマト運輸のクロネコメンバーズの会員登録を進めるまでの過程を見てみましょう。
@<br>{}@<img>{yahoo_fido_1}はクロネコメンバーズのログイン画面です。
Yahoo! JAPANのログインボタンを押すことでID連携がスタートします。
//image[yahoo_fido_1][RPのID連携ボタンがある画面 @<href>{https://cmypage.kuronekoyamato.co.jp/portal/custtempregpage, クロネコメンバーズ ページより}][scale=0.5]

まずはOpenId Provider側の認証を行う必要があります。@<img>{yahoo_fido_2}はOpenId ProviderであるYahoo! JAPANのログイン画面です。
未ログイン状態からログインするためにIDを入力して「次へ」ボタンを押します。
ここで使用するアカウントはすでにWebAuthnでの登録フローが完了しているアカウントになります。
(すでにYahoo! JAPANI IDでログイン済みのブラウザを使っている場合はIDを入力する必要はなく、そのまま@<img>{yahoo_fido_3}に飛びます)
//image[yahoo_fido_2][OpenId Providerのログイン画面 @<href>{https://login.yahoo.co.jp, login.yahoo.co.jp より}][scale=0.5]

「次へ」ボタンが押された瞬間に裏側ではchallengeの要求が走り、サーバーからはchallengeを含んだ必要なoptionsが返却されます。
そして、credentials.get()が叩かれ、ブラウザに対して認証命令を送ります。
この認証命令がブラウザに送られ、認証器（今回の場合はAndroid内の認証器）が呼び出され、@<img>{yahoo_fido_3}のローカル認証として指紋認証が要求されています。
//image[yahoo_fido_3][FIDOの認証で指紋が要求される画面][scale=0.5]

指紋認証に成功したら、サーバー側に対してassertionが送られ、署名されたデータをOpenId ProviderのFIDOサーバーに送り、保存してある公開鍵で検証を行います。

ここで認証のフローは終わり、Token発行のフローに入ります。

その後、認可コードを発行するためにOpenId Providerにアクセスします。このときにRP(ヤマト運輸)に対して、
どのような属性情報を渡すのか同意を取る画面が現れ、
ユーザーはどこまでの情報をRPに渡していいのかを確認します。

//image[yahoo_fido_4][RPに対してどのような属性情報を渡すのかの同意画面][scale=0.5]

属性情報の同意が取れましたら、裏では@<img>{fido_in_oidc}のように認可コードが格好された後、各種Tokenが払い出され、Access Tokenを使用して属性情報を取得します。
そして、取得された属性情報はRP(ヤマト運輸)に渡されます。
モザイクばかりになってしまいましたが、処理が進むと@<img>{yahoo_fido_5}のように登録画面にOpenId Provider(Yahoo! JAPAN)に保存されていた属性情報が埋め込まれた登録画面が現れます。

//image[yahoo_fido_5][RPの登録時に属性情報がプリセットされている画面][scale=0.7]

このようにID連携をすることで事前に必要な情報を取得することができ、ユーザーが登録をしやすいような土台をRP(ヤマト運輸)はユーザーに提供することができます。

以上の具体例からもOpenID ConnectとFIDOは共存することが可能であることがわかります。

== FIDOとOpenID ConnectでのRelying Partyについて
私がFIDOを一番最初に学んだときに一番最初に引っかかった部分がここでした。

突然ですが、先程見てみたAuthorizationフローにFIDOのフローを追加した図(@<img>{fido_in_oidc})をもう一度見てみましょう。

@<img>{fido_in_oidc}の中にはRelying Party(RP)という単語が2回出てきます。
OpenID ConnectとFIDOの両方で登場するこの単語なのですが、２つの仕様で指し示すサービスが違います。

OpenID Connectの中ではID連携を行いたいサービスをRPと呼び、FIDOでは主にID Provider(OpenID ConnectではOpenId Providerとなっている部分)のことをRPと呼びます。
同じID関連の仕様なのに同じ単語で違う意味合いになっているのはなぜでしょうか？

このRPが二箇所で出てきてしまっている問題を考えるにあたって、RPの元々の意味を考える必要が出てきます。
ID関連の仕様で使われているRPの元々の意味を「認証(本人検証)を委任しているサービス」と捉えることにより、この疑問は解消されるはずです。
（ここではわかりやすく考えるためOpenID Provider = ID Providerと考えてください）

@<b>{OpenID Connectの場合}
//lead{
  ID連携を行いたいサービス（Relying Party）がOpenId Providerに対して認証(本人検証)を委任している
//}
@<b>{FIDOの場合}
//lead{
  ID Provider(Relying Party)が認証器(Authenticator)に対して認証(本人検証)を委任している
//}
という風に解釈できます。

よくあるパスワードでのログインをする場合などにおいてID Providerが認証(本人確認)を委任することは基本的にはないのですが、
FIDOの仕様ではID Providerが認証（本人確認）を認証器(Authenticator)に委任しています。

そのためOpenID Connectの枠組みにFIDOを導入しようと思った場合、認証(本人確認)は@<img>{oidc-fido-rp}のように委任を繰り返します。
//image[oidc-fido-rp][それぞれのRPの関係]

FIDOを学び始めたとき、最初は同じ意味を指している単語だと思い、自分の解釈が間違っていると思いRPという単語で混乱していました。
@<br>{}しかし、RPという言葉の意味をよく考えてみると矛盾なく、同じ単語でサービスが違っているのかも理解することができます。


== FIDOとSelf-issuedについて
OpenID Connectの仕様の中にも認証をどうするかについて書かれた@<b>{Self-issued}という仕様があります。

参考：@<href>{https://openid.net/specs/openid-connect-core-1_0.html#SelfIssued}

この仕様を簡単に説明すると

 1. OpenID Providerの認証機能をローカルデバイスの中に持っていき、認証を行う
 2. ユニークな鍵ペアを作成して安全に管理する
 3. ローカルデバイスに保存された秘密鍵で署名されたId Tokenの検証を行うことで認証とする
 4. Issuerは https://self-issued.me が使用される
などが挙げられます。

実際にOpenID Connectの枠組みを使い、Self-issuedを取り入れている企業としてはリクルートが有名です。

さて、先程簡単に説明させていただいた特徴をどこかで見たことがないでしょうか。
そうです。FIDO認証で使われている仕組みによく似ています。

実際にサービスとして運用しているリクルートの仕組みでも端末側で鍵ペアを作成したり、
公開鍵をサーバー上で管理したり、PKCE(Proof Key for Code Exchange by OAuth Public Clients)
を用いてチャレンジレスポンス方式を使いリプレイアタックを防いだりなど、
大雑把な技術的な構成だけを考えたらFIDOとあまり変わらないことを行っています。

もちろんパラメーターの渡し方などに関しては大きく違います。
あくまで、公開鍵暗号方式を用いてログインをするという点に関してはとても良く似ているという意味です。

では、FIDOとSelf-issuedの違いは何でしょうか？
私自身もここに関してはわからない部分が多かったので、今回有識者にヒアリングなどをしつつ自分なりに考えてみました。

まず、FIDOとSelf-issuedを考えるときは、技術的な視点ではなくFIDOやOpenID Connectの枠組みを考える必要があります。
先程から何回か出ていますが、OpenID ConnectはID連携の仕様、FIDOは認証の仕様という感じにそれぞれの立場(レイヤー)が違います。

Self-issuedはあくまでID連携から派生した仕様であり、基本的にはID Tokenや認可トークンをどのように発行するかが重要で、認証はそのための手段です。
なので、Self-issuedを使いたい場合もOpenID Connectの枠組みなので本質としては属性情報をやり取りすることが目的になります。
そのため、公開鍵暗号を使って認証を行っていますが、その情報自体はトークンのやり取りの中に内包されて仕様ができています。

一方FIDOは認証に特化しています。認証におけるエコシステムやフレームワークをFIDO Allianceが標準仕様を考えて導入を推し進めています。
そのため、鍵ペアを作成し秘密鍵で署名を行う認証器についてや、サーバー側でのAttestationの検証方法に関しても厳密に仕様が決まっています。

なので、FIDOとSelf-issuedに関して簡単に言ってしまうと
//lead{
@<b>{手段は一緒だけど、そもそもの目的が違う}
//}
だと思われます。

私見になってしまいますが、実際にサービス導入を考えていく上では一長一短あると思います。
厳密な認証に注目したフレームワークやエコシステムの導入を考えている場合はFIDOを組み込むことを考えますし、
OpenID Connectの仕様内のトークンのやり取りの中で公開鍵暗号方式の認証を行いたい場合はSelf-issuedを導入することも考えられると思います。

導入に際して考えるべきは技術的な優位性を考えるよりも、
現状でどのような企業が導入を進めているかだったり、認定制度がどうなっているかだったり、
ユーザー（この場合はエンドユーザーだけでなくOpenID ConnectでのRelying Partyも含む）に
導入してもらえるのはどちらなのかなどの、マーケットの成熟度を考える必要があるかもしれません。

== まとめ
FIDOとOpenID Connectの関係性を中心にお話させていただきました。
OpenID Connectなどを使ってID連携をしているサービスはたくさんあります。
それらのサービスでも現在運用しているID連携の枠組みを維持しながらFIDOのような
新しい認証体験をユーザーに提供することが可能だということが少しでも伝わりましたでしょうか。

FIDOやOpenID Connectのような認証認可の仕様は、
より強固でより利便性の高く、さまざまな分野で使ってもらえるような仕様へと日々アップデートしています。
特にFIDOのWebAuthnなどはサービス導入が始まったばかりなので、
いろいろなサービスが使い始めたら今後もどんどん発展していくでしょう。
そのときにはきっとFIDOとOpenID Connectの関連性を考えるときがくると思います。
その時の手助けに今回の記事が参考になったら幸いです。

=== 参考資料
 * @<href>{https://techblog.yahoo.co.jp/advent-calendar-2018/webauthn/,Yahoo! JAPANでの生体認証の取り組み（FIDO2サーバーの仕組みについて）}
 * @<href>{https://developer.yahoo.co.jp/yconnect/v2/authorization_code/,Yahoo! ID連携　Authorization Codeフロー}
 * @<href>{https://qiita.com/ritou/items/a9649bd76ccd7aa50d90,OpenID Connect Self-Issued OP(IdP) の仕様と応用}
 * @<href>{https://openid-foundation-japan.github.io/openid-connect-core-1_0.ja.html,OpenID Connect Core 1.0 incorporating errata set 1}
 * @<href>{https://speakerdeck.com/rtechkouhou/openid-connect-self-issued-idpwoying-yong-sitasingle-sign-onfalseshi-zhuang,OpenID Connect　Self-Issued IdPを応用したSingle Sign Onの実装}

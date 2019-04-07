= ビットコインの技術で WebAuthn のデバイスロスに対抗する

== デバイスロスについて

WebAuthn では認証に用いる秘密鍵がデバイスに紐づき、デバイスの外には洩れないことにより安全性を担保しています。
これはユーザーに対して「デバイスを持っていれば認証ができる」という画一的なユーザーエクスペリエンスを提供する一方、
デバイスの紛失がすなわち認証情報の紛失となり、その無効化方法やリカバリー方法がひとつの課題となっています。

WebAuthn ではプライバシーを担保するために、それぞれのサービスの認証に、異なるキーペアを利用します。
また Authenticator の ID などのひとつのキーを特定する識別子もありません。
その為、登録時にユーザーがそれぞれの Authenticator に名前を付ける、あるいはスマートフォンの場合であればユーザーエージェントから名前を取り込むといった運用になるかと思います。

//image[w-services][キーに名前を付けることは可能だが… *Googleのセキュリティーキー一覧画面より][scale=0.5]

単一のサービスであれば、ユーザーがデバイスを紛失した際に、入力した名前などをもとに Authenticator を無効化するといった処理が可能かもしれませんが、複数のサービスに登録していた場合はどうでしょうか。
ユーザーがデバイスを紛失してしまうと、なくしたデバイスがどのサービスで利用していたかを判断することは、ユーザーから判別することは難しくなります。

== アカウントリカバリーの方法

デバイスロスの課題については、よくアカウントのリカバリー方法と共に語られます。
当然デバイスを紛失した際には、新しいデバイスに対してユーザーを紐づける、アカウントリカバリーが必要となります。
しかし安全にアカウントリカバリーを行うにはどうすればいいのでしょうか。

たとえば普段の認証に WebAuthn を利用したとしても、デバイスをなくした際の再登録にユーザーID と秘密の質問だけで復元できるのであれば、当然セキュリティレベルは「秘密の質問」のレベルに引っ張られます。
したがってサービスのセキュリティを担保するには、アカウントリカバリーの方法も含めて考える必要があります。(当然それだけではありませんが)

その為、サービスが利用ユーザーをいかに正しく確認するかという手続きが必要になってきます。
KYC(Know Your Customer) と呼ばれ、金融分野では以前からサービスを提供するにあたり重要な考え方だと認識されてきましたが、
近年、様々な企業がペイメントサービスを提供しはじめ、それに伴い再び注目されることになっているようです。

そんな中 WebAuthn の GitHub issue 「Recovering from Device Loss」@<fn>{GitHubIssue} では、デバイスロスについての議論が繰り広げられています。

//footnote[GitHubIssue][https://github.com/w3c/webauthn/issues/931]

== Recovery Credentials Extension

その中で Yubico のエンジニアである emlum @<fn>{emlum} が Recovery Crendentials Extension を 疑似スペックのドラフト@<fn>{recovery-extension-draft}
として公開しています。
非常に興味深いドラフトだったため、emlum にドラフトの詳細を教えてほしいとコメントしたところ、安全性が確認できる前に実装されてしまうというリスクがあるため、暗号化の専門家による吟味が終わるまではまだ公開するつもりはないという旨の返事が返ってきました。

と、いうことで、公開するつもりがないならしょうがないので、自分で想像して書いてみました、というのが今回のテーマです。

//footnote[recovery-extension-draft][Pseudo-spec draft: https://gist.github.com/emlun/74a4d8bf53fd760a5c5408b418875e2b]

//footnote[emlum][emlum@Yubico https://gist.github.com/emlun]

=== ドラフトの概要

ドラフトでは、Authenticator の登録時に、リカバリー用の Recovery Authenticator の情報を Extension に含め、事前に CredentialId や、公開鍵をサーバーに送信します。
登録時の流れとしては、次のようになります。

 1. 事前に Recovery Authenticator を、Main Authenticator に登録 （登録処理のたびに state をインクリメント）
 2. Client の credentials.create() コマンドに、"recovery" Extension を action = generate で実行
 3. Client は通常の登録フローに加え、 Recovery Credential の id, 公開鍵, state をサーバーに送信
 4. 通常の登録処理に加え、recovery Extension に含まれる Recovery Credential のリストをサーバーに保存  @<fn>{recovery_cred_save_with_main_auth} 


//footnote[recovery_cred_save_with_main_auth][リカバリー用の Credential はメインの Authenticator に紐づいて保存される]

//image[w-registration][Recovery Authenticator の登録]

次にリカバリーの流れです。想定としては、通常は「キーの紛失」リンクから、メールアドレスを入力しサービスから送信されるリカバリーフローへのリンクから開始することになるかと思います。

 1. サーバーはどの Authenticator をリカバリするかをユーザーに選択させる画面を表示し、ユーザーはなくした Authenticator を選択
 2. サーバーはなくした Authenticator に紐づけられている recovery Credentials のリストを Extension に含めて送信
 3. Client は credentials.create() コマンドを recovery Extension の action = "recover", allowCredentials = [ recovery credential] で実行
 4. リカバリー用の Authenticator は allowCredentials に自身の CredentialId が含まれていれば、対応する秘密鍵で署名を返す
 5. サーバーは公開鍵で署名を検証し、検証が完了すれば Recovery 用の Authenticator の情報を通常の Authenticator としてユーザーに紐づける
 6. なくした Authenticator を無効化

//image[w-authentication][Recovery Authenticator を利用できるように設定]


=== WebAuthn のキーペアについて

本題に入る前に、なぜ WebAuthn の Authenticator のバックアップが困難なのかをあらためて確認しましょう。

=== 秘密鍵はデバイスの中に

WebAuthn はパスワードレス認証と呼ばれるはそもそもパスワードがなぜ危険だったのでしょうか。
パスワードの危険性は、攻撃者が取得することが容易であること、そして誰でも利用できることでした。
それに対して、WebAuthn （正確には FIDO の範疇ですが）では、秘密鍵をデバイスに閉じ込めることを選択しました。
いままで誰でも利用可能だったクレデンシャルを、デバイスに紐づけることによってデバイスがなければ認証ができなくしたのです。

Authenticator のバックアップが可能ということは、すなわちデバイスから秘密鍵が漏れること、あるいはデバイスが複製可能であるということになります。
これはデバイスを所持していないと認証ができないという大前提を破壊してしまう行為であるといえます。

もっとも筆者はバックアップが可能であることが、悪だとは考えておりません。
世の中にはバックアップ可能な FIDO デバイスも存在しており、今回のアカウントリカバリーの方法は、そのデバイスの仕組みが大きなヒントになりました。



== HD ウォレットの仕組み

Authenticator のバックアップが難しいことが分かったところで、先ほど話していた秘密鍵をバックアップ可能な Authenticator をご紹介します。

//image[w-ledger][Ledger Nano S * Ledger SAS. https://shop.ledger.com/ より引用][scale=0.5]

=== 楕円曲線暗号の数学的性質

楕円曲線暗号は、楕円曲線を有限の点の集合 (有限体) とし、をを利用した、離散対数問題の困難性を安全性の根拠とする暗号です。

楕円曲線暗号では、曲線上の点 @<i>{P} には有限体における足し算が定義されています。  @<fn>{italic} 
@<i>{P1} + @<i>{P2} = @<i>{P3} は、@<img>{w-curve-add} のように @<i>{P1} と P2 と接線が楕円曲線と重なる点の y = 0 の軸を中心に反転させた点と定義されます。


//footnote[italic][イタリックで表現している文字は点、通常の文字はスカラーです。]

//image[w-curve-add][@<i>{P1} + @<i>{P2} = @<i>{P3}][scale=0.5]

次に単位元 @<i>{O} を定義します。@<i>{O} は曲線平面上の y 軸と並行な直線と楕円曲線が交わる無限遠点です。
つまり、ある点 @<i>{P} に @<i>{O} を加算すると、x = 0 の直線を軸に、対称な点が @<i>{P'}, そのまた対象は @<i>{P} ということで、 @<i>{P} + @<i>{O} = @<i>{P} となります。

最後に点の掛け算です。
楕円曲線上の点について、スカラー倍は、曲線と点の接線を足し算同様に計算することで計算できます。

//image[w-curve-mul][2 * @<i>{P} = @<i>{P}  + @<i>{P} ][scale=0.5]

さて、このようにスカラー倍を定義した際に、ある点 @<i>{G} を始点として、 1G, 2G, 3G... と求めていくと、有限な n について
大して n@<i>{G} = @<i>{O} となる n が存在します。さらに n@<i>{G} + 1@<i>{G} = @<i>{G} ですから、1@<i>{G} ～ n@<i>{G} までの n個の点が、楕円曲線上の点すべての点となります。
#@# これを有限巡回群とよび n を位数とよびます。ビットコインなどで利用されている楕円曲線のスペックである、secp256k1 では 2@<sup>{256}－2@<sup>{32}－2@<sup>{9}－2@<sup>{8}－2@<sup>{7}－2@<sup>{6}－2@<sup>{4}－1 という大きな素数を位数とする
細かい解説は省略しますが、楕円曲線暗号では k * @<i>{P} の計算は簡単な一方、@<i>{P} が明らかなばあいに
@<i>{P} = k * @<i>{G}  @<fn>{basepoint} における k の値を求めることが難しいことを利用して暗号として利用します。@<fn>{how_to_sign} 


//footnote[how_to_sign][実際に署名として利用するには、少し工夫が必要ですが、今回はライブラリに頼ります。]

//footnote[basepoint][G は、通常定められた楕円曲線上の点（basepoint）]

=== ECDSA を使ってみる

では実際に ECDSA を利用してみましょう。
今回のサンプルコードはすべて python3 で記述されています。なお、すべてのコードは GitHub @<fn>{samplecode} にアップロードしていますので適宜参考にしてください。

はじめに必要ライブラリをインストールします。

//cmd{
pip install ecdsa
//}


以下は 32 bytes の秘密鍵から、ECDSA SECP256k1 の ECDSA 鍵を生成し、署名と検証を行うサンプルです。

//listnum[ecdsa_signature.python][ecdsa ライブラリで署名と検証][python]{
import ecdsa

prikey_str = bytes.fromhex(
 '1bab84e687e36514eeaf5a017c30d32c1f59dd4ea6629da7970ca374513dd006'
)
prikey = ecdsa.SigningKey.from_string(prikey_str, curve=ecdsa.SECP256k1)

print('prikey: ',prikey_str.hex())
# prikey:  1bab84e687e36514eeaf5a017c30d32c1f59dd4ea6629da7970ca374513dd006

data = b'hello'
sign = prikey.sign(data)
print('sign  : ', sign.hex()) 
# sign  :  82f15f67976b1b397eac6b13235220c4b6a32f75db03bd5....

pubkey = prikey.get_verifying_key()

print("pub   : ",  pubkey.to_string().hex())
# pub   :  18684cfb6aefc8a7e4c08b4bad03fcd167c6e7401fe8099....

print("verified: ", pubkey.verify(sign, data))
# verified:  True

//}

//footnote[samplecode][https://www.github.com/watahani/hd-authenticator]

=== HD ウォレット

HD ウォレットは BIP0032@<fn>{BIP0031} で定義されているビットコインのウォレット管理プロトコルです。
HD は Hierarchical Deterministic（階層的決定性）の頭文字をとったものです。
ひとつのシードから複数の秘密鍵を作成できるのですが、階層的決定性とあるように、階層的に秘密鍵を生成できます。
また、同じシードからは同じ秘密鍵の階層を作成可能です。

bitcoin ウォレットのプロトコルでは、マスターキーを @<i>{m} ,そこの子を @<i>{m/x}, その子を @<i>{m/x/y} と呼ぶため、本書でもそのように呼称します。

//image[w-hd][階層的な鍵生成]

//footnote[BIP0031][https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki]

=== バックアップ用 Authenticator

=== サンプルコード
=== 考慮事項

== 活用例

== 最後に

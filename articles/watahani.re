= ビットコインの技術で WebAuthn のデバイスロスに対抗する

#@# TODO 初めに

== デバイスロスについて

WebAuthn では認証に用いる秘密鍵がデバイスに紐づき、デバイスの外には洩れないことにより安全性を担保しています。
これはユーザーに対して「デバイスを持っていれば認証ができる」という画一的なユーザーエクスペリエンスを提供する一方、
デバイスの紛失がすなわち認証情報の紛失となり、その無効化方法やリカバリー方法がひとつの課題となっています。

WebAuthn ではプライバシーを担保するために、それぞれのサービスの認証に、異なるキーペアを利用します。
また Authenticator の ID などのひとつのキーを特定する識別子もありません。
その為、登録時にユーザーがそれぞれの Authenticator に名前を付ける、あるいはスマートフォンの場合であればユーザーエージェントから名前を取り込むといった運用になるかと思います。
例えば Google アカウントで新しく Authenticator を登録する際には、@<img>{w-google-seckeys}のように登録デバイスに名前をつけ、どのデバイスを登録したのかをユーザーが理解できるようにしています。

ユーザーがデバイスを紛失した際は、入力した名前などをもとに Authenticator を無効化するといった処理が可能かもしれませんが、単一のサービスではなく、複数のサービスに登録していた場合はどうでしょうか。
ユーザーがデバイスを紛失してしまうと、なくしたデバイスがどのサービスで利用していたか、あるいは、登録した Authenticator のうち、なくしたデバイスはどれなのかを判断することは、ユーザーから判別することは難しくなります。

//image[w-google-seckeys][キーに名前を付けることは可能だが… *Googleのセキュリティーキー一覧画面より][scale=0.5]

== アカウントリカバリーの方法

ユーザーが登録した Authenticator や、サービスが分からなくなることよりも大きな問題があります。
それはデバイスを紛失した際の、アカウントリカバリーの問題です。

FIDO や WebAuthn において、デバイスロスの課題は、よくアカウントのリカバリー方法と共に語られます。
当然デバイスを紛失した際には、新しいデバイスに対してユーザーを紐づける、アカウントリカバリーが必要となります。
しかし安全にアカウントリカバリーを行うにはどうすればいいのでしょうか。

たとえば普段の認証に WebAuthn を利用したとしても、デバイスをなくした際の再登録にユーザーID と秘密の質問だけで復元できるのであれば、当然セキュリティレベルは「秘密の質問」のレベルに引っ張られます。
したがってサービスのセキュリティを担保するには、アカウントリカバリーの方法も含めて考える必要があります。(当然それだけではありませんが)

その為、サービスが利用ユーザーをいかに正しく確認するかという手続きが必要になってきます。
KYC（Know Your Customer） と呼ばれ、金融分野では以前からサービスを提供するにあたり重要な考え方だと認識されてきましたが、
近年、さまざまな企業がペイメントサービスを提供しはじめ、それに伴い再び注目されることになっているようです。

しかしながら本書では、アカウントのリカバリーを KYC に頼ることなく、ユーザー自身の力で行えるかもしれない仕様について解説します。

//footnote[GitHubIssue][https://github.com/w3c/webauthn/issues/931]

== Recovery Credentials Extension

WebAuthn の GitHub issue 「Recovering from Device Loss」@<fn>{GitHubIssue} では、デバイスロスについての議論が繰り広げられています。
その中で Yubico のエンジニアである emlum @<fn>{emlum} が Recovery Crendentials Extension を 疑似スペックのドラフト@<fn>{recovery-extension-draft}
として公開しています。
非常に興味深いドラフトだったため、emlum にドラフトの詳細を教えてほしいとコメントしたところ、安全性が確認できる前に実装されてしまうというリスクがあるため、暗号化の専門家による吟味が終わるまではまだ公開するつもりはないという旨の返事が返ってきました。

と、いうことで、公開するつもりがないならしょうがないので、自分で想像して書いてみました、というのが今回のテーマです。

//footnote[recovery-extension-draft][Pseudo-spec draft: https://gist.github.com/emlun/74a4d8bf53fd760a5c5408b418875e2b]

//footnote[emlum][emlum@Yubico https://gist.github.com/emlun]

=== ドラフトの概要

WebAuthn のスペックには Extension と呼ばれる拡張データが定められています。
例としては、認証情報に位置情報を含める Location Extension、認証メソッドを判別する User Verification Method Extension、あるいは U2F との互換性のために Client に実装されている FIDO AppID Extension などがあります。
今回のドラフトでは、recovery Extension という名前の Extension が提案されています。
これは Authenticator の登録時に、リカバリー用の Recovery Authenticator の情報を Extension に含め、事前に CredentialId や、公開鍵をサーバーに送信し、
紛失時にキーの再登録なしにリカバリーが可能になるというものです。

主な登場人物は次のとおりです。

 * Recovery Key: リカバリー用の Authenticator です。事前に Main Key に登録しておきます。
 * Main Key: 普段利用している Authenticator で、今回紛失します。RP への登録処理時にリカバリーキーの情報も登録します。
 * RP: 認証を行うサービス提供者です。Main Key と Recovery Key の公開鍵を保存し、通常は Main Key で登録します。

登録時の流れとしては、次のようになります。

 1. 事前に Recovery Authenticator を、Main Authenticator に登録 （登録処理のたびに state をインクリメント）
 2. Client の credentials.create() コマンドに、"recovery" Extension を action = generate で実行
 3. Client は通常の登録フローに加え、 Recovery Credential の id, 公開鍵, state をサーバーに送信
 4. 通常の登録処理に加え、recovery Extension に含まれる Recovery Credential のリストをサーバーに保存  @<fn>{recovery_cred_save_with_main_auth} 


//footnote[recovery_cred_save_with_main_auth][リカバリー用の Credential はメインの Authenticator に紐づいて保存される]

//image[w-registration][Recovery Authenticator の登録]

次にリカバリーの流れです。
想定としては、通常は「キーの紛失」リンクから、メールアドレスを入力しサービスから送信されるリカバリーフローへのリンクから開始することになるかと思います。

 1. サーバーはどの Authenticator をリカバリーするかをユーザーに選択させる画面を表示し、ユーザーはなくした Authenticator を選択
 2. サーバーはなくした Authenticator に紐づけられている recovery Credentials のリストを Extension に含めて送信
 3. Client は credentials.create() コマンドを recovery Extension の action = "recover", allowCredentials = [ recovery credential] で実行
 4. リカバリー用の Authenticator は allowCredentials に自身の CredentialId が含まれていれば、対応する秘密鍵で署名を返す
 5. サーバーは公開鍵で署名を検証し、検証が完了すれば Recovery 用の Authenticator の情報を通常の Authenticator としてユーザーに紐づける
 6. なくした Authenticator を無効化

//image[w-authentication][Recovery Authenticator を利用できるように設定]

=== 技術課題

ドラフトの概要を見るとよくできたプロトコルのように思えます。
ユーザーは事前にリカバリー用の Authenticator を登録しておくことで、デバイスを無くした際に、追加の認証をすることなくデバイスのリカバリー処理を行います。
まるでパスワードの再設定のように、デバイスの再設定が可能になるのです。
さらに、リカバリーを行った場合には古いデバイスは自動で削除されるため、デバイスの無効化も同時に行えます。

しかしちょっと待ってください。リカバリー用の Authenticator を登録するとはどういうことでしょうか。
WebAuthn のスペックを理解している方なら、リカバリー用の Authenticator を登録するにはいくつかの問題があると想像がつくでしょう。

リカバリー用の Authenticator を登録するにはどうすればよいのか、
本題に入る前に、なぜ WebAuthn の Authenticator のバックアップが困難なのかを、あらためて確認しておきましょう。

==== 秘密鍵はデバイスの中に

パスワードの危険性は、攻撃者が取得することが容易であること、そして誰でも利用できることでした。
それに対して、WebAuthn （正確には FIDO の範疇ですが）では、秘密鍵をデバイスに閉じ込めることを選択しました。
いままで誰でも利用可能だったクレデンシャルを、デバイスに紐づけることによってデバイスがなければ認証ができなくしたのです。

Authenticator のバックアップが可能ということは、すなわちデバイスから秘密鍵が漏れること、あるいはデバイスが複製可能であるということになります。
これはデバイスを所持していないと認証ができないという大前提を破壊してしまう行為であるといえます。

当然リカバリー用の Authenticator を登録する際にも、秘密鍵をエクスポートするわけにはいきません。では、公開鍵を登録すれば解決するのでしょうか。

==== サービスごとの異なるキーペア

結論からいうと、公開鍵をただエクスポートしただけではリカバリー用のキーは作成できません。
なぜなら WebAuthn ではアカウントごと、アプリケーションごとに異なるキーペアを利用するようになっているからです。

YubiKey のようなデバイスでは多くの場合、デバイスを登録する際に新しいキーペアを生成し、公開鍵をサーバーに送信します。
秘密鍵の生成にはデバイスに埋め込まれた device secret（認証に用いる秘密鍵とは異なる）を利用します。
device secret に対応する公開鍵だけでは、キーペアは当然作れませんし、かといって device secret をエクスポートする行為は論外です。

ですので、リカバリー用のキーがメインのデバイスに渡す情報は、秘密鍵がなくとも、アプリケーションごとの公開鍵を作成できる情報である必要があります。

== HD ウォレットの仕組み

Authenticator のバックアップが難しいことが分かったところで、先ほど話していた秘密鍵をバックアップ可能な Authenticator をご紹介します。

//image[w-ledger][Ledger Nano S * Ledger SAS. https://shop.ledger.com/ より引用][scale=0.5]

#@# TODO キーの紹介
===[column]  Ledger Nano S の紹介
Ledger Nano S は、HDウォレットを実装しているビットコインウォレットのひとつです。
ビットコインやイーサリアムといった仮想通貨のウォレットとしてだけではなく、FIDO U2F のキーとしても動作します。
面白いことに、HDウォレットの仕組みから、ある seed をもとに Authenticator のリストアが可能です。
U2F のキーについても、 seed さえ忘れなければ同じキーが復元可能です。
Authenticator としてみた場合にも、とても面白い仕組みといえるでしょう。（安全性に関する議論はここでは触れません。）

本作を書くにあたり、この HD ウォレットの仕組みが非常に参考になりました。
#@# もっとも筆者はバックアップが可能であることが、悪だとは考えておりません。
#@# 世の中にはバックアップ可能な FIDO デバイスも存在しており、今回のアカウントリカバリーの方法は、そのデバイスの仕組みが大きなヒントになりました。
===[/column]

結論からいうと、この HD ウォレットの仕組みが Authenticator のリカバリープロトコルに利用できると考えています。
つまり、「秘密鍵がなくとも、アプリケーションごとの公開鍵を生成できる」仕組みを HD ウォレットは備えているのです。

その為、この章では、HD ウォレットの仕組みについて少し解説していきます。

HD ウォレットは BIP0032@<fn>{BIP0032} で定義されているビットコインのウォレット管理プロトコルです。
HD ウォレットは Hierarchical Deterministic（階層的決定性）ウォレットの略で、ひとつのシードから複数の秘密鍵を作成できるのですが、階層的決定性とあるように階層的に秘密鍵を生成できます。
@<img>{w-hd} にあるようにマスターキーを m として、m の子 m/x, またその子 m/x/y のように階層的に秘密鍵を作成できます。@<fn>{hierarchical}また、同じシードからは同じ秘密鍵の階層を作成可能です。

//image[w-hd][階層的な鍵生成] 


//footnote[BIP0032][https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki]

//footnote[hierarchical][bitcoin ウォレットのプロトコルでは、マスターキーを @<i>{m} ,そこの子を @<i>{m/x}, その子を @<i>{m/x/y} と呼ぶため、本書でもそのように呼称します。]


=== 楕円曲線暗号の数学的性質

HD ウォレットについて理解するには、少しばかり楕円曲線暗号について知る必要があります。
ここで覚えていただきたいことは、
楕円曲線暗号の秘密鍵が大きな整数で、ある楕円曲線上の点を秘密鍵の整数倍した点が公開鍵であること、
公開鍵が楕円曲線上のある点であり、点同士の足し算が可能であるということだけです。

楕円曲線暗号は、楕円曲線を有限の点の集合 (有限体) とし、離散対数問題の困難性を安全性の根拠とする暗号です。
これだけでは意味が分からないので、よくある楕円曲線のグラフを用いて解説します。

このあたりの解説については、多くのサイトで解説されているため、より詳しく知りたい方はご自身で検索してみてください。

楕円曲線暗号では、曲線上の点 @<i>{P} には有限体における足し算が @<img>{w-curve-add} のように定義されています。@<fn>{italic} 
@<i>{P1} + @<i>{P2} = @<i>{P3} は、@<img>{w-curve-add} のように @<i>{P1} と P2 と接線が楕円曲線と重なる点の y = 0 の軸を中心に反転させた点と定義されます。

//footnote[italic][イタリックで表現している文字は点、通常の文字はスカラーです。]

//image[w-curve-add][@<i>{P1} + @<i>{P2} = @<i>{P3}][scale=0.5]

次に単位元 @<i>{O} を定義します。@<i>{O} は曲線平面上の y 軸と並行な直線と楕円曲線が交わる無限遠点です。
@<img>{w-curve-add} を用いて説明すると、
ある点 @<i>{P2} に @<i>{O} を加算した点を考えます。まず、無限遠点との交線である、@<i>{P2O}と交わる点、これはすなわち @<i>{P2'} = @<i>{P3}です。
その対称な点は当然 @<i>{P2} ですので、 つまり @<i>{P2} + @<i>{O} = @<i>{P2} となります。

最後に点の掛け算です。
楕円曲線上の点について、スカラー倍は、曲線と点の接線を足し算同様に計算することで計算できます。

//image[w-curve-mul][2 * @<i>{P} = @<i>{P}  + @<i>{P} ][scale=0.5]

このように定義することで、楕円曲線上の点に対するスカラー倍が定義できました。
さて、このようにスカラー倍を定義した際に、ある点 @<i>{G} を始点として、 1G, 2G, 3G... と求めていくと、
有限な n に対して n@<i>{G} = @<i>{O} となる n が存在します。さらに n@<i>{G} + 1@<i>{G} = @<i>{G} ですから、1@<i>{G} ～ n@<i>{G} までの n個の点が、楕円曲線上の点すべての点となります。

#@# これを有限巡回群とよび n を位数とよびます。ビットコインなどで利用されている楕円曲線のスペックである、secp256k1 では 2@<sup>{256}－2@<sup>{32}－2@<sup>{9}－2@<sup>{8}－2@<sup>{7}－2@<sup>{6}－2@<sup>{4}－1 という大きな素数を位数とする
細かい解説は省略します（解説できません）が、楕円曲線暗号では k * @<i>{P} の計算は簡単な一方、@<i>{P} が明らかなばあいに
@<i>{P} = k * @<i>{G}  @<fn>{basepoint} における k の値を求めることが難しいことを利用して暗号として利用します。@<fn>{how_to_sign} 

小難しい解説をしましたが、あくまでここで覚えていただきたいのは、楕円曲線暗号が、公開鍵（楕円曲線上の座標）の足し算が可能であること、
曲線上のある点 @<i>{G} から何倍移動したか @<i>{k} が秘密鍵となる点です。

//footnote[how_to_sign][実際に署名として利用するには、少し工夫が必要ですが、今回はライブラリに頼ります。]

//footnote[basepoint][G は、通常定められた楕円曲線上の点（basepoint）]

=== ECDSA を使ってみる

では実際に ECDSA を利用してみましょう。
今回のサンプルコードはすべて python3 で記述されています。なお、すべてのコードは GitHub @<fn>{samplecode} にアップロードしていますので適宜参考にしてください。 @<fn>{note} 

//footnote[samplecode][https://www.github.com/watahani/hd-authenticator]

//footnote[note][しばらくサンプルコードが続きますが、ひとつのファイルである前提です。]

はじめに必要ライブラリをインストールします。

//cmd{
pip install ecdsa
//}


以下は 32 bytes の秘密鍵から、ECDSA SECP256k1 の ECDSA 鍵を生成し、署名と検証を行うサンプルです。
このように ECDSA ではランダムな数字から秘密鍵を生成することが可能です。

//listnum[ecdsa_signature.py][ecdsa ライブラリで署名と検証][python]{
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


HD ウォレットの説明に入る前に、ふたつの関数を定義しておきます。
秘密鍵の加算と公開鍵（楕円曲線上の座標）の加算です。

まずは秘密鍵同士の加算用関数です。
秘密鍵はベースポイント @<i>{G} を何倍したか、という値でしたので単に整数として足すだけです。
ただし、楕円曲線の位数（order）を超えてはいけないので curve.order の余剰を返します。入力チェックなどは省略しています。

//listnum[sample1][秘密鍵の加算][python]{
from math import log2

def add_secret_keys(*args, order):
    ''' add two prikey as int and return private key of ecdsa lib'''
    prikey = 0

    for key in args:
        if prikey == 0:
            prikey = int.from_bytes(key, "big")
        else:
            prikey = (prikey + int.from_bytes(key, "big")) % order

    return prikey.to_bytes( int(log2(order)/8), 'big')


k1 = '1bab84e687e36514eeaf5a017c30d32c1f59dd4ea6629da7970ca374513dd006'
k2 = '375709cd0fc6ca29dd5eb402f861a6583eb3ba9d4cc53b4f2e1946e8a27ba00c'
key1 = bytes.fromhex(k1)
expect = bytes.fromhex(k2)

result = add_secret_keys(key1, key1, order=ecdsa.SECP256k1.order )
print(result.hex())
# 375709cd0fc6ca29dd5eb402f861a6583eb3ba9d4cc53b4f2e1946e8a27ba00c

print(result == expect)
# True
//}


次に公開鍵の加算用関数ですが、すでに ecdsa ライブラリの Point クラスで定義されているため、そちらを利用します。
@<list>{sample3} は公開鍵 k1p の座標 p に p　を加算して新しい公開鍵を生成しています。
ecdsa ライブラリの Point クラスには __add__ 関数がすでに定義されているので  p + p のように記述すれば大丈夫です。
公開鍵の座標は VerifyingKey.pubkey.point で取得できます。
座標から公開鍵へは ecdsa.VerifyingKey.from_public_point() を利用します。

//listnum[sample3][公開鍵の加算][python]{
k1 = bytes.fromhex(
  '1bab84e687e36514eeaf5a017c30d32c1f59dd4ea6629da7970ca374513dd006'
)


k1p = ecdsa.SigningKey.from_string(k1, curve=ecdsa.SECP256k1).get_verifying_key()

p = k1p.pubkey.point

p2 = p + p

k2p = ecdsa.VerifyingKey.from_public_point(p2, curve=ecdsa.SECP256k1)

print(k2p.to_string().hex())
# e0cf532282ef286226bec....
# k2 = k1*2

k2 = bytes.fromhex(
  '375709cd0fc6ca29dd5eb402f861a6583eb3ba9d4cc53b4f2e1946e8a27ba00c'
)

k2p_from_prikey = ecdsa.SigningKey.from_string(k2, curve=ecdsa.SECP256k1)
                                  .get_verifying_key()

print(p2 == k2p_from_prikey.pubkey.point)
# True

//}

ここで注目すべきは、秘密鍵 '1bab84e687....' から生成した公開鍵 @<b>{k1p} の座標 p を加算した @<b>{p2} が、秘密鍵 '375709cd0f....' から生成した公開鍵 p2_from_prikey の座標と等しいことです。
実は楕円曲線暗号では k1*@<i>{G} + k2*@<i>{G} = (k1 + k2)*@<i>{G} が成り立ちます。
この性質はDH鍵交換などのアルゴリズムを学んだ方には馴染み深いものだと思いますが、この後の HD ウォレットの説明には必須の知識ですのでしっかり覚えておいてください。

=== マスター秘密鍵の作成

話を HD ウォレットに戻しましょう。HD ウォレットでは、あるシードから秘密鍵を次々作成可能でした。
具体的にどのように生成するかを説明します。まずマスター秘密鍵を作成するため、 seed から HMAC-SHA512 を計算し、その左 256 bit を秘密鍵として利用します。
@<img>{w-master_keygen} が実際のフローです。

//image[w-master_keygen][マスター秘密鍵の生成フロー]

コードにしたものが、@<list>{master_keygen} です。

hmac512() は、key と source から HMAC-SHA512 を計算する関数です。BIP0032 では "Bitcoin seed" がキーとするよう決められているそうですが、ここでは "webauthn" をキーとしています。
prikey_and_ccode() は、与えられた key と seed から、HMAC-SHA512 を計算し、HMAC の左 256bit から楕円曲線 SECP256k1 の秘密鍵を、残りの 256bit を ccode として返します。
ccode はチェーンコードと呼ばれるもので、ここではマスターキーのチェーンコードですのでマスターチェーンコードと呼びます。

@<list>{master_keygen} で生成される値を @<img>{w-master_keygen} に記載しましたので、見比べてみてください。

//listnum[master_keygen][マスター秘密鍵の生成][python]{
import hmac
import hashlib
import ecdsa

def hmac512(key, source):
    if isinstance(source, str):
        source = source.encode()
    if isinstance(key, str):
        key = key.encode()
    return hmac.new(key, source, hashlib.sha512).digest()

def prikey_and_ccode(key, seed):
    ''' generate ECDSA private key of ecdsa lib and chain code as string'''
    hmac = hmac512(key, seed)
    prikey = hmac[:32]
    prikey = ecdsa.SigningKey.from_string(prikey, curve=ecdsa.SECP256k1)
    ccode = hmac[32:]
    return prikey, ccode

m_key, m_ccode = prikey_and_ccode('webauthn', 'techbookfest')
m_pubkey = m_key.get_verifying_key()

print("m_prikey:", m_key.to_string().hex())
# m_prikey: b681f32891f35b55034fc26d0317bffaf7b0ecc0f4058ca221e4bfc991cb4470

print("m_pubkey:", m_pubkey.to_string().hex())
# m_pubkey: 4a467119fc2a0638eb762677fca69f6c92e8bd36dff87f30c553e4764c5fe10...

print("m_ccode :", m_ccode.hex())
# m_ccode : 39c759f2df91af229f2237ea6ed9eb102da188e68bcdab3b2913b215bfeae030

//}

=== 子秘密鍵の作成

次にチェーンコードと秘密鍵から子秘密鍵を生成します。フローは少し複雑ですが、利用するのは先ほどと同じ HMAC-SHA512 です。
先ほどと同じく @<list>{key_generation_flow} で実際に生成される値と同じ値を @<img>{w-key_generation_flow} に書き込みましたので、よく見比べてください。

子秘密鍵を生成する流れを少し説明します。
子秘密鍵は親秘密鍵と、親チェーンコードから作られます。
今回は m_key（b681...）と m_ccode（39c7...）から作ります。

まず m_key の公開鍵 m_pubkey を計算します。これは m_key.get_verifying_key().to_string() で求められます。
次に index を作成します。バイト配列にする必要があるので、なんとなく 4bytes になるよう x0000 を作成します。
そして公開鍵 m_pubkey と index を結合し、結合したものを seed, 親チェーンコード m_ccode を key として HMAC-SHA512 を計算します。
生成した左半分を deltakey（秘密鍵） として、親秘密鍵と足し合わせます。
足し合わせたものを、子秘密鍵 m_0_prikey, HMAC-SHA512 の残り半分を、子チェーンコード m_0_ccode として保存します。

//image[w-key_generation_flow][子秘密鍵の作成]

//listnum[key_generation_flow][子秘密鍵の生成][python]{
def deltakey_and_ccode(index, pubkey, ccode):
    source = pubkey + index
    deltakey, child_ccode = prikey_and_ccode(key=ccode, seed=source)
    return deltakey, child_ccode

def child_key_and_ccode(index, prikey, ccode):
    ''' generate childkey from prikey and chain code'''
    pubkey = prikey.get_verifying_key().to_string()
    deltakey, child_ccode = deltakey_and_ccode_from(index, pubkey, ccode)
    print("deltakey  : ", deltakey.to_string().hex())
    
    child_key = add_secret_keys(
                    prikey.to_string(),
                    deltakey.to_string(),
                    order=SECP256k1.order
                )
    child_key = ecdsa.SigningKey.from_string(child_key, curve=SECP256k1)
    return child_key, child_ccode

index = 0
index = index.to_bytes(4,'big')

deltakey, _ = deltakey_and_ccode(index, m_pubkey.to_string(), m_ccode)

m_0_key, m_0_ccode = child_key_and_ccode(index, m_key, m_ccode)
print("deltakey  :", delta_key.hex())
# deltakey  : 7a822fd6977e9011e3b4b116b2143ab64c92605c1c373dcd33bf643074bba2af

print("m_0_prikey:", m_0_key.to_string().hex())
# m/0 prikey: 310422ff2971eb66e7047383b52bfab28994703660f42a3395d1c56d3650a5de

print("m_0_pubkey:", m_0_key.get_verifying_key().to_string().hex())
# m/0 pubkey:  adef0692801bed2606510b9eb1680d7b02882c88def3760851bc8e3ec152bd0a...


print("m_0_ccode :", m_0_ccode.hex())
# m/0 ccode : 96524759775e8d3bb80858ef8e975311aa0a10e8f55d4596bf2e8c21cb37d047

//}


同様に子秘密鍵から、さらに子秘密鍵を作成することができ、まさに @<img>{w-hd} のように階層構造のキーペアを生成可能です。
実際のフローは @<img>{w-cc_keygen_flow} となります。

//image[w-cc_keygen_flow][子秘密鍵と子チェーンコードから、さらにキーを生成]

//listnum[hd_authenticator_4][子秘密鍵の子を作成][python]{
index = 1
index = index.to_bytes(4, 'big')
m_0_1_key, m_0_1_ccode = child_key_and_ccode(index, m_0_key, m_0_ccode)

print("m/0/1 prikey: ", m_0_1_key.to_string().hex())
# m/0/1 prikey: 4bf0f511bbf7bfbbcc8da0c91c33d77c66d86e6c249f60e1c5f10a943504b9a4

print("m/0/1 pubkey: ", m_0_1_key.get_verifying_key().to_string().hex())
# m/0/1 pubkey:  9d63574b6578babeb3c7b21bccbbc6ff3cd0de3391b662f14bebdb94706c03bc...

print("m/0/1 ccode :", m_0_1_ccode.hex())
# m/0/1 ccode : 6f14f270f19c7ac300f7fc5bbb6274ed974f36b4b5ecac60244a33d71a821043
//}

=== 拡張公開鍵

さて、ここまでで、ひとつの秘密鍵と、ひとつのチェーンコードから無限のキーペアを生成できることがわかりました。
ここで子秘密鍵の生成フロー @<img>{w-cc_keygen_flow} をもう一度よく見てみましょう。
今回、HDウォレットを解説したのは「秘密鍵がなくても、アプリケーションごとの公開鍵を生成できる」仕組みを知るためでした。
では @<img>{w-cc_keygen_flow} から秘密鍵を取り除くとどうなるでしょうか？

実は秘密鍵がなくとも、子公開鍵であれば生成可能です。
実際に計算してみましょう。

@<b>{m_0_pubkey} と @<b>{m_0_ccode} から @<b>{m_0_1_pubkey} と @<b>{m_0_1_ccode} を生成します。

まず、@<b>{m_0_1_ccode} は簡単です。そもそも m_0_pubkey と index を連結したものを seed に、 m_0_ccode を key として HMAC-SHA512 をとったものが @<b>{deltakey} と @<b>{m_0_1_ccode} でした。
次に注目すべきは、m_0_1_pubkey が m_0_1_deltakey と m_0_prikey を加算した m_0_1_prikey の公開鍵である点です。

@<list>{sample3} で解説したとおり、楕円曲線暗号では k1*@<i>{G} + k2*@<i>{G} = (k1 + k2)*@<i>{G} が成り立ちます。
ここでは秘密鍵 m_0_1_prikey と m_0_1_deltakey を加算したものが m_0_1_prikey ですから、次の式が成り立ちます。@<fn>{add_point} 

 * m_0_prikey*@<i>{G} + m_0_1_deltakey*@<i>{G} = (m_0_1_prikey)*@<i>{G} = m_0_1_pubkey

ここで

 * m_0_prikey*@<i>{G} = m_0_pubkey
 
ですから、

 * m_0_1_pubkey = m_0_pubkey + m_0_1_deltakey*@<i>{G}

と計算でき、秘密鍵を使わずに、子鍵の公開鍵とチェーンコードを生成することができました。

//footnote[add_point][実際に加算するのは公開鍵そのものではなく、座標です]

実際のコードは @<list>{extended_pubkey} です。
今回も（少しわかりにくいですが）全体のフロー図を、実際の値と共に載せていますので理解の助けにしてください。

//listnum[extended_pubkey][公開鍵とチェーンコードから子公開鍵を作成][python]{
index = 1
index = index.to_bytes(4, 'big')

m_0_pubkey = m_0_key.get_verifying_key()

m_0_1_deltakey, m_0_1_ccode = deltakey_and_ccode(index, m_0_pubkey.to_string(), m_0_ccode)
m_0_1_delta_pubkey = m_0_1_deltakey.get_verifying_key()
print("delta_pubkey:", m_0_1_delta_pubkey.to_string().hex())
# delta_pubkey: f2f2584425210aae1deca1803d019b941115a46d16c7d1cdf4279617da6e2f1b76...

m_0_1_deltakey_point = m_0_1_delta_pubkey.pubkey.point
m_0_1_point = m_0_pubkey.pubkey.point + m_0_1_deltakey_point

m_0_1_pubkey = ecdsa.VerifyingKey.from_public_point(m_0_1_point, curve=SECP256k1)
print("m/0/1_pubkey:",m_0_1_pubkey.to_string().hex())
# m/0/1_pubkey: 9d63574b6578babeb3c7b21bccbbc6ff3cd0de3391b662f14bebdb94706c03bc...
print("m/0/1 ccode :", m_0_1_ccode.hex())
# m/0/1 ccode : 6f14f270f19c7ac300f7fc5bbb6274ed974f36b4b5ecac60244a33d71a821043
//}

//image[w-extended_pubkey][公開鍵とチェーンコードから子公開鍵を作成するフロー]

これで秘密鍵を利用せずに、公開鍵とチェーンコードから子公開鍵と子チェーンコードを生成する仕組みが理解できたと思います。
実は BIP0032 で定義されている拡張公開鍵は、公開鍵とチェーンコードと index, depth, などをまとめて管理するフォーマットなのです。
ただし、今回はあくまで WebAuthn の Extension として利用するだけなので、フォーマットには触れず、その仕組みだけを拝借します。

== バックアップ用 Authenticator の作成

では実際に WebAuthn で利用できるバックアップ Authenticator を作成してみましょう。
コードの全体は GitHub を参照ください。

//listnum[hdkey][HD Authenticator のサンプルコード 重要な関数のみ抽出][python]{
class HDKey(object):
    ''' extended key '''
    #@# def __init__(self,keyid, prikey, ccode, pubkey, is_prikey, parentId=None, depth=0):...

    #@# def _checksum(self, source, appid_hash=None):...

    #@# def _generateRandomKeyId(self, appid_hash=None):...

    #@# def _child_key_from_id(self, keyid, appid_hash=None):...

    #@# def sign(self, source):...

    def app_prikey(self, credid, appid_hash):
        if not self.is_prikey:
            raise Exception('this key does not prikey') 

        if len(credid) == CRED_ID_LENGTH:
            childkey = self._child_key_from_id(credid[:KEY_ID_LENGTH])
            prikey = childkey._child_key_from_id(credid[KEY_ID_LENGTH:], appid_hash)
            return prikey
        else:
            return None

    def pubkey_seed(self):
        child_keyid = self._generateRandomKeyId()
        return self._child_key(child_keyid,include_prikey=False)

    def app_pubkey(self, appid_hash):
        if not self.depth == 1:
            raise Exception('app pubkey should be generated by child key')
        elif not appid_hash:
            raise Exception('required appid_hash to generate app pubkey')
        else:
            child_keyid = self._generateRandomKeyId(appid_hash=appid_hash)
            return self._child_key(child_keyid,include_prikey=False)

    def is_child_key_id(self, keyid, appid_hash=None):
        keyid_L = keyid[:HALF_KEY_ID_LENGTH]
        keyid_R = keyid[HALF_KEY_ID_LENGTH:]

        return keyid_R == self._checksum(keyid_L, appid_hash=appid_hash)
//}

HDKey@<list>{hdkey} は Authenticator 内のキージェネレーターだと考えてください。
HDKey は sign() メソッドと verify() メソッドがあり、署名と署名の検証が可能です。

また HDウォレットのキー同様、HDKey 自身から階層的にキーペア（あるいは公開鍵）を生成可能です。今回は WebAuthn の recovery Extensions で利用できるように 3つの関数を定義しました。

pubkey_seed() は、マスターキーから公開鍵を生成する pubkey_seed （公開鍵シード）を作成します。pubkey_seed は「秘密鍵は含まないが、その子公開鍵を生成できる」、HD ウォレットでいう拡張公開鍵です。
app_pubkey() は公開鍵シードから各アプリケーションごとの公開鍵を生成する関数です。引数には appid_hash を指定します。

appid_hash は keyid と共に pubkey_seed のチェーンコードをキーとして HMAC を生成し、credentialID に含めて出力しています。credentialID に appid_hash を含めて HMAC をとった値が入っていることにより、
この後 Master Key と credentialID から秘密鍵を復元する際、credentialID と appid の検証に利用します。

最後に app_prikey() は WebAuthn の credentialID から、秘密鍵を含むアプリケーションごとの HDKey を生成します。
この際引数に appid_hash を同時に渡すことで CredentialID が本当にこの HDKey で生成されたものかを判別します。

全体的な流れは @<img>{w-hdkey_flow} のようになります。

//image[w-hdkey_flow][HDKey クラスによる recovery フロー]

===　マスターキーの生成

実際にそれぞれのキーを生成して、署名や検証が可能か確認してみましょう。
まずはデバイスのマスターキーを生成します。


//listnum[masterkey][マスターキーの生成][python]{
m_key, m_ccode = prikey_and_ccode('webauthn', 'seed')
master_key_index = 0
master_key = HDKey(keyid=master_key_index.to_bytes(0, 'big'), prikey=m_key, ccode=m_ccode, pubkey=None, is_prikey=True)

# マスターキーは recovery Key から外には出ない
master_key.print_debug()

# ======== master_key ==========
# is_prikey: True
# depth    : 0
# keyid    : 
# prikey   : c0efe2a00cfe3d31fe84b0d72366842392fe374730d02dcc50e690284fafa863
# pubkey   : 323cea14302640267a9db642c9fab532167e5ef64d2b878dd4cb8b09251feb17...
# credid   : 
# ccode    : f96fe3c225726a7ee001dcd98349593a76f797ec5cde9abff844cb55ebf9f506
//}

秘密鍵が含まれていること、ccode が存在することがわかります。

=== Public Key seed の作成

マスターキーから Public Key seed を作成します。
先ほど拡張公開鍵を作成した方法と同じアルゴリズムで、チェーンコードを含む公開鍵を生成します。
実際のコードは@<list>{pubkey_seed} です。

//listnum[pubkey_seed][Public Key seed の生成][python]{
print("======== pubkey_seed ==========")
pubkey_seed = master_key.pubkey_seed()

pubkey_seed.print_debug()
# ======== pubkey_seed ==========
# is_prikey: False
# depth    : 1
# keyid    : b66b0b6a66fce126869e3d5042169886a6e832e631350f356bad2f22d026ca62
# prikey   : None
# pubkey   : 356cddf81e91cbdeaed452a988c5fd9b4c36e24d5a6b916dc87cb10be239b6e0...
# credid   : b66b0b6a66fce126869e3d5042169886a6e832e631350f356bad2f22d026ca62
# ccode    : 0753795ad0c1be808005b008ee6f0d670641eb8c641cd1790cc4e3a0eb815be5
//}

先ほどの拡張公開鍵を作成した時と異なるのは、keyid というパラメータを生成している点です。
keyid を生成する際には、ランダムナンスを半分の長さで生成します。ここでは 128bit 長の nonce を加算して新しい公開鍵を生成しています。  @<fn>{random} 
次に、自身のチェーンコードを利用して nonce から hmac512 を計算し、その先頭 128bit をchecksum として返します。
そして nonce と checksum を合成したものを keyid として返します。

//footnote[random][ランダムナンスから keyid などを生成するので、ここからは毎回実行結果が変わります。]


//listnum[keyid][keyid の生成メソッド][python]{
class HDKey(object):
    ...
    def _checksum(self, source, appid_hash=None):
        if appid_hash:
            s = source + appid_hash
        else:
            s = source
        return hmac512(self.ccode, s)[:HALF_KEY_ID_LENGTH]

    def _generateRandomKeyId(self, appid_hash=None):
        keyid_L = secrets.token_bytes(HALF_KEY_ID_LENGTH)
        return keyid_L + self._checksum(keyid_L, appid_hash)
//}

ここで生成した拡張公開鍵は、普段利用している Main Key に保存されます。

=== アプリケーションごとの公開鍵を作成

Main Key は自身を RP に登録する際に、Recovery Key が利用する公開鍵（app_pubkey）を生成し、Extension に含む形でサーバーに送ります。
@<list>{app_pubkey} は Main Key の内部で、登録時に行われるものだと考えてください。

//listnum[app_pubkey][アプリケーションごとの公開鍵を作成][python]{
# Main Authenticator の登録時に、リカバリー用のキーが利用予定の公開鍵を作成し、同時に RP に登録する。

print("======== app_pubkey ==========")
appid = 'https://example.com'

appid_hash = hashlib.sha256(appid.encode()).digest()

app_pubkey=pubkey_seed.app_pubkey(appid_hash)

app_pubkey.print_debug()
# ======== app_pubkey ==========
# is_prikey: False
# depth    : 2
# keyid    : 5a2e5e1d4616b3f7bf824c27e88d3953d9619d6e6d7df10d74ca14ef47959792
# prikey   : None
# pubkey   : fff4ba46d2348c48dbe99bef1c8bf99b6f02513c58562f7f12f8969fb0c6737f...
# credid   : b66b0b6a66fce126869e3d5042169886a6e832e631350f356bad2f22d026ca62...
# ccode    : 1a8872c6da749a52bc2f8a3eee8a30c7c985c2e8349185e0c226a32401dfc119
//}

ここで、credid（WebAuthn の credentialID）は、親の Public Key seed の keyid に自身の keyid を合成したバイト配列です。
先ほど 128bit の nonce と 128bit の checksum で 256bit の keyid を生成しました。したがって、credid はその 2 倍、512bit の長さになります。 

マスターキーがもつ、秘密鍵とマスターチェーンコードがあれば、keyid から子秘密鍵を生成することが可能です。
すなわち、credentialID があれば、マスターキーから Public Key seed に対応する秘密鍵が、そして app_pubkey に対応する秘密鍵も生成可能になります。

=== CredentialID から秘密鍵を復元

では、credentialID から秘密鍵を復元してみましょう。

Main Key を紛失してしまい、アカウントリカバリーフローが始まったと考えてください。
サーバーは、リカバリー用に登録されている credentialID を Recovery Key に送信します。
なお CredentialID は Main Key 上で生成されており、Recovery はマスターキーを持っています。

//listnum[app_prikey][アプリケーションごとの秘密鍵を復元][python]{
print("======== private key ==========")

prikey = master_key.app_prikey(app_pubkey.credid, appid_hash)

prikey.print_debug()

source = 'nonce'.encode()
sign = prikey.sign(source)
result = app_pubkey.verify(sign, source)
# ======== private key ==========
# is_prikey: True
# depth    : 2
# keyid    : 5a2e5e1d4616b3f7bf824c27e88d3953d9619d6e6d7df10d74ca14ef47959792
# prikey   : a4f3db3bbde43b46e0878917d42ca362c1d6d8abe598f308f53e70f42c25f6f3
# pubkey   : fff4ba46d2348c48dbe99bef1c8bf99b6f02513c58562f7f12f8969fb0c6737f...
# credid   : b66b0b6a66fce126869e3d5042169886a6e832e631350f356bad2f22d026ca62...
# ccode    : 1a8872c6da749a52bc2f8a3eee8a30c7c985c2e8349185e0c226a32401dfc119
//}

@<list>{app_prikey} で生成された app_prikey を確認してください。まず、 is_prikey = True であり、秘密鍵を含んでいることが分かります。
さらに公開鍵が、先ほど生成した app_pubkey の公開鍵 '1a887...' と等しいことが分かるかと思います。
マスターキーを持っている、Recovery Key 内では、対応する credentialID があれば、秘密鍵を復元することが可能なのです。

@<list>{recovery_prikey} で、もう少し細かく秘密鍵の復元方法を見てみましょう。まずマスターキーは credentialID を受け取ると、その長さをチェックし半分に分割します。
そして、その前半部分から _child_key_from_id() メソッドで、公開鍵 pubkey_seed に対応する秘密鍵を生成します。
これは Main Key に送ったアプリケーションの公開鍵を生成する拡張公開鍵に対応する秘密鍵を含んだ HDKey です。
つまりこの HDKey から、署名用の秘密鍵を生成することが可能です。

次に、credentialID の残りの半分を、署名用の秘密鍵の _child_key_from_id() メソッドに appid_hash と共に渡します。
このとき、 _child_key_from_id メソッド内では、is_child_key_id() メソッドで、与えられた credentialID と appid_hash が本当に、
マスターキーから生成されたものかをチェックしています。
マスターキーで生成された credentialID 意外を指定した場合、 invalid keyid という例外が投げられます。

//listnum[recovery_prikey][秘密鍵の生成コード][python]{

    def app_prikey(self, credid, appid_hash):
        if not self.is_prikey:
            raise Exception('this key does not prikey') 

        if len(credid) == CRED_ID_LENGTH:
            childkey = self._child_key_from_id(credid[:KEY_ID_LENGTH])
            prikey = childkey._child_key_from_id(credid[KEY_ID_LENGTH:], appid_hash)
            return prikey
        else:
            return None

    def _child_key_from_id(self, keyid, appid_hash=None):
        if self.is_child_key_id(keyid, appid_hash):
            return self._child_key(keyid,include_prikey=self.is_prikey)
        else:
            raise Exception('invalid keyid {}'.format(keyid.hex()))
//}

この credentialID をチェックする機構は YubiKey が credentialID を生成する仕組みを参考に実装しました。


=== 署名の検証

では、最後に署名を生成し検証してみます。
ここで、公開鍵である app_pubkey は RP に、先ほど生成した app_prikey はリカバリー用のキー内部にあると考えてください。

//listnum[verifying][署名の作成と検証][python]{
source = 'nonce'.encode()
sign = prikey.sign(source)
result = app_pubkey.verify(sign, source)

print("========   result   ==========")

print('souce :','nonce')
print('pubkey:', app_pubkey.pubkey.to_string().hex())
print('sign  :', sign.hex())
print('result:', result)

# ========   result   ==========
# souce : nonce
# pubkey: fff4ba46d2348c48dbe99bef1c8bf99b6f02513c58562f7...
# sign  : bd5e8f09f821240db155ca35935f022e852cd7d06093f62...
# result: True
//}

@<list>{verifying} では、'nonce' という文字列に対し、app_prikey で署名を行っています。
なお、ドラフトでは authenticatorData から、Extensions を除いたものと、clientDataHash を結合したものに対して署名を行っていますが、今回はあくまでコンセプトの説明ですので単純な文字列に対して署名を行いました。

最後の検証では、サーバーに保存されている pubkey を利用して署名を検証できていることが分かります。

このように、リカバリー用のキーは自身が生成する署名用の秘密鍵に対応する公開鍵を、秘密の情報は共有せずに RP と共有することができました。
また、その秘密鍵を、RP から送られてくる credentialID から復元することができました。

=== 考慮事項

== 活用例

== 最後に

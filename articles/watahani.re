= ビットコインの技術で WebAuthn のデバイスロスに対抗する

#@# TODO 初めに

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

=== 技術課題

ドラフトの概要を見るとよくできたプロトコルのように思えます。
ユーザーは事前にリカバリー用の Authenticator を登録しておけば、デバイスを無くした際に、追加の認証をすることなくまるでパスワードの再設定のようにリカバリー処理を行います。
さらに、リカバリーを行った場合には古いデバイスは自動で削除されるため、デバイスの無効化も同時に行えます。

しかしちょっと待ってください。リカバリー用の Authenticator を登録するとはどういうことでしょうか。

本題に入る前に、なぜ WebAuthn の Authenticator のバックアップが困難なのかをあらためて確認しましょう。

==== 秘密鍵はデバイスの中に

パスワードの危険性は、攻撃者が取得することが容易であること、そして誰でも利用できることでした。
それに対して、WebAuthn （正確には FIDO の範疇ですが）では、秘密鍵をデバイスに閉じ込めることを選択しました。
いままで誰でも利用可能だったクレデンシャルを、デバイスに紐づけることによってデバイスがなければ認証ができなくしたのです。

Authenticator のバックアップが可能ということは、すなわちデバイスから秘密鍵が漏れること、あるいはデバイスが複製可能であるということになります。
これはデバイスを所持していないと認証ができないという大前提を破壊してしまう行為であるといえます。

当然リカバリー用の Authenticator を登録する際にも、秘密鍵をエクスポートするわけにはいきません。では、公開鍵を登録すれば解決するのでしょうか。

#@# コラムにする？
もっとも筆者はバックアップが可能であることが、悪だとは考えておりません。
世の中にはバックアップ可能な FIDO デバイスも存在しており、今回のアカウントリカバリーの方法は、そのデバイスの仕組みが大きなヒントになりました。

==== サービスごとの異なるキーペア

結論からいうと、公開鍵をただエクスポートしただけではリカバリー用のキーは作成できません。
なぜなら WebAuthn ではアカウントごと、アプリケーションごとに異なるキーペアを利用するようになっているからです。

YubiKey のようなデバイスでは多くの場合、デバイスを登録する際に新しいキーペアを生成し、公開鍵をサーバーに送信します。
秘密鍵の生成にはデバイスに埋め込まれた device secret（認証に用いる秘密鍵とは異なる）を利用します。
device secret に対応する公開鍵だけでは、キーペアは当然作れませんし、かといって device secret をエクスポートする行為は論外です。

ですので、リカバリー用のキーがメインのデバイスに渡す情報は、秘密鍵は含まないもののアプリケーションごとの公開鍵を作成できる情報である必要があります。

== HD ウォレットの仕組み

Authenticator のバックアップが難しいことが分かったところで、先ほど話していた秘密鍵をバックアップ可能な Authenticator をご紹介します。

//image[w-ledger][Ledger Nano S * Ledger SAS. https://shop.ledger.com/ より引用][scale=0.5]

#@# TODO キーの紹介

HD ウォレットは BIP0032@<fn>{BIP0031} で定義されているビットコインのウォレット管理プロトコルです。
HD ウォレットは Hierarchical Deterministic（階層的決定性）ウォレットの略で、ひとつのシードから複数の秘密鍵を作成できるのですが、階層的決定性とあるように階層的に秘密鍵を生成できます。
@<img>{w-hd} にあるようにマスターキーを m として、m の子 m/x, またその子 m/x/y のように階層的に秘密鍵を作成できます。@<fn>{hierarchical}また、同じシードからは同じ秘密鍵の階層を作成可能です。

//image[w-hd][階層的な鍵生成] 


//footnote[BIP0031][https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki]

//footnote[hierarchical][bitcoin ウォレットのプロトコルでは、マスターキーを @<i>{m} ,そこの子を @<i>{m/x}, その子を @<i>{m/x/y} と呼ぶため、本書でもそのように呼称します。]

結論からいうと、この HD ウォレットの仕組みが Authenticator のリカバリープロトコルに利用できると考えています。
つまり、「秘密鍵は含まないものの、アプリケーションごとの公開鍵を生成できる」仕組みを HD ウォレットは備えているのです。

その為、この章では、HD ウォレットの仕組みについて少し解説していきます。
といっても、細かい仕組みは理解する必要は無いので（筆者も理解していません）安心してください。

ではどのように HD ウォレットがキーペアの階層構造を生成するのかを見ていきましょう。

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

#@# TODO コード上げる
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

=== マスター秘密鍵の作成

話を HD ウォレットに戻しましょう。HD ウォレットでは、あるシードから秘密鍵を次々作成可能でした。
具体的には seed から HMAC-SHA512 を計算し、その左 256 bit を秘密鍵として利用します。


//listnum[hd_authenticator.py(1)][マスター秘密鍵の生成][python]{
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

print("m_prikey: ", m_key.to_string().hex())
# m_prikey:  b681f32891f35b55034fc26d0317bffaf7b0ecc0f4058ca221e4bfc991cb4470

print("m_pubkey: ", m_pubkey.to_string().hex())
# m_pubkey:  4a467119fc2a0638eb762677fca69f6c92e8bd36dff87f30c553e4764c5fe10b7365c5190d5176d23c3956c8d0d5c2bb05406a0acdde77ab57d03851fe2b6646

print("m_ccode : ", m_ccode.hex())
# m_ccode :  39c759f2df91af229f2237ea6ed9eb102da188e68bcdab3b2913b215bfeae030

//}

hmac512() は、key と source から HMAC-SHA512 を計算する関数です。BIP0032 では "Bitcoin seed" がキーとするよう決められているそうですが、ここでは "webauthn" をキーとしています。
prikey_and_ccode() は、与えられた key と seed から、HMAC-SHA512 を計算し、HMAC の左 256bit から楕円曲線 SECP256k1 の秘密鍵を、残りの 256bit を ccode として返します。
ccode はチェーンコードと呼ばれるもので、ここではマスターキーのチェーンコードですのでマスターチェーンコードと呼びます。


=== 子秘密鍵の作成

子秘密鍵を作成する前に、秘密鍵同士の加算用関数を定義しておきます。秘密鍵はベースポイント @<i>{G} を何倍したか、という値でしたので単に整数として足すだけです。
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

key1 = bytes.fromhex('1bab84e687e36514eeaf5a017c30d32c1f59dd4ea6629da7970ca374513dd006')
key2 = bytes.fromhex('1bab84e687e36514eeaf5a017c30d32c1f59dd4ea6629da7970ca374513dd006')
expect = bytes.fromhex('375709cd0fc6ca29dd5eb402f861a6583eb3ba9d4cc53b4f2e1946e8a27ba00c')

add_secret_keys(key1, key2, order=SECP256k1.order ) == expect
# True

//}



//listnum[hd_authenticator_3][子秘密鍵の生成][python]{

def deltakey_and_ccode_from(index, pubkey, ccode):
    source = pubkey + index
    deltakey, child_ccode = prikey_and_ccode(key=ccode, seed=source)
    return deltakey, child_ccode

def child_key_and_ccode_from(index, prikey, ccode):
    ''' generate childkey from prikey and chain code'''
    pubkey = prikey.get_verifying_key().to_string()

    delta_key, child_ccode = deltakey_and_ccode_from(index, pubkey, ccode)

    child_key = add_secret_keys(prikey.to_string(), delta_key.to_string(), order=SECP256k1.order)
    child_key = ecdsa.SigningKey.from_string(child_key, curve=SECP256k1)
    return child_key, child_ccode

index = 0
index = index.to_bytes(4,'big')

m_0_key, m_0_ccode = child_key_and_ccode_from(index, m_key, m_ccode)

print("m_0_prikey: ", m_0_key.to_string().hex())
# m/0 prikey:  310422ff2971eb66e7047383b52bfab28994703660f42a3395d1c56d3650a5de

print("m_0_pubkey: ", m_0_key.get_verifying_key().to_string().hex())
# m/0 pubkey:  adef0692801bed2606510b9eb1680d7b02882c88def3760851bc8e3ec152bd0ac6d187b85b082e215fa4b7c4f3b86ddc7382b35728bd6a6f0424d03f99ed2206


print("m_0_ccode : ", m_0_ccode.hex())
# m/0 ccode :  96524759775e8d3bb80858ef8e975311aa0a10e8f55d4596bf2e8c21cb37d047

//}


//listnum[hd_authenticator_4][子秘密鍵の子を作成][python]{
index = 1
index = index.to_bytes(4, 'big')
m_0_1_key, m_0_1_ccode = child_key_and_ccode_from(index, m_0_key, m_0_ccode)

print("m/0/1 prikey: ", m_0_1_key.to_string().hex())
# m/0/1 prikey:  4bf0f511bbf7bfbbcc8da0c91c33d77c66d86e6c249f60e1c5f10a943504b9a4

print("m/0/1 pubkey: ", m_0_1_key.get_verifying_key().to_string().hex())
# m/0/1 pubkey:  9d63574b6578babeb3c7b21bccbbc6ff3cd0de3391b662f14bebdb94706c03bcee1061395a9e1ec0f90734fb6129c8238da352380089052ccb54c723ca60ef47

print("m/0/1 ccode : ", m_0_1_ccode.hex())
# m/0/1 ccode :  6f14f270f19c7ac300f7fc5bbb6274ed974f36b4b5ecac60244a33d71a821043
//}

=== 拡張公開鍵



== バックアップ用 Authenticator の作成

===　Public Key seed の作成

=== CredentialID を作成する

=== CredentialID から秘密鍵を復元

=== 考慮事項

== 活用例

== 最後に

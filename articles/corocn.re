= Ruby on Rails + Vue.js を使ったパスワードレス認証とその実装

@corocnです。
普段はRuby on Rails （RoR）+ Vue.jsで小さめのWebサービスを書いていることが多いです。
ガッツリ仕事で認証関連の仕事をしているわけではなく、セキュリティエンジニアでもありませんのであしからず。
認証が好きです。

今回の私が書く話はWebAuthnの実アプリケーションの組み込みに関して、
WebAuthn APIの呼び出し部分や署名検証部分等の細かい仕様にフォーカスしてある記事はたくさんあるのですが、
じゃあこれを実際のシングルページアプリケーション（SPA）にどう組み込むんだ？という疑問がフツフツと湧いてきたので、
自分なりの考えを整理してみて実装してみたのが今回の話です。

FIDO2、WebAuthnが何者なのかという説明は、書きたいことの本質ではないので省きます。（多分他の誰かが書いてるだろうし。）
もし復習しておきたいという方は、@agektmr 氏の
「パスワードの不要な世界はいかにして実現されるのか - FIDO2 と WebAuthn の基本を知る」@<fn>{agektmr_fido_document}が非常によくまとまっているのでオススメでした。

//footnote[agektmr_fido_document][パスワードの不要な世界はいかにして実現されるのか - FIDO2 と WebAuthn の基本を知る： @<href>{https://blog.agektmr.com/2019/03/fido-webauthn.html} ]

==[column] パスワードレス認証を実装する必要があるのか

私のようなBtoCサービス開発者がパスワードレス実装を必要があるのか？というそもそも論から話すと、
基本的にパスワードレスを考慮しなくていいんじゃないの？と考えています。

今のFIDO2関連の動きを見ていると、ほぼ確実にGoogle等のIdentity Provider（IdP）はFIDO2対応してくるでしょう。
アプリケーションの認証機能を外部のIdPを利用したSingle Sign On（SSO）で実現しておけば、何もしなくてもFIDO2対応は完了です。そもそも多くの状況で認証機能は、提供したいサービスの本質的なところではないので、
最初から認証部分を外部サービスに丸投げというのは大いにありだと思っています。

認証機能はサービスの本質ではないと述べましたが、パスワードレスによってユーザー体験が向上すれば、
ユーザー登録や認証部分での脱落がなくなるというメリットはあります。
私はすでにユーザー名+パスワード認証のみのサービスに登録するのが面倒でしょうがなく、
多くのサービスの登録部分で脱落する人間の一人です。

現時点で、この新しい認証の体験は、他所のサービスに比べて優位性となりますが、
パスワードレスが当たり前となった世界では、これが「あたりまえ品質」になります。
既存のユーザー名+パスワード認証のみのサービスは、時代に取り残されていくでしょう。
ユーザーを囲い込むために、自前のIdPを保有しているサービスはたくさんありますが、今後頑張って対応する必要がありますね。

==[/column]

== 今回試した構成

実装の話に入る前に、今回の話の技術スタックを紹介しておきます。
すでに沢山の方がWebAuthnのサンプルアプリケーションを作成されていますが、
SPA + APIの構成でやっている例が見当たらなかったのでトライしてみました。

 * Backend: Ruby on Rail API Mode v5.2.2 
 * Fronend: Nuxt.js（Vue.js）v2.4.0

若干新しめな構成です。フロントエンド側は楽したい一心でNuxt.jsを使ってはじめましたが、
正直そこまで重い実装ではないので、素のVue.jsで書いてもよいかと思います。
鍵の登録から検証まで試すぐらいであれば、コンポーネント1個作れば十分なレベルですので。
今回は最終的にNuxt.jsのプラグインとして実装できました。
余裕があればRails側もDeviseのようなGem化しても面白かったかも。

サンプルコードは次に置いておきます。モノレポでfrontディレクトリ配下にNuxt.jsを格納しています。

 * @<href>{https://github.com/corocn/webauthn-rails-playground}

== 実装のポイント

実装のポイントは2点です。

 * セッションを使わず実装する
 * Server Requirements に沿ったエンドポイント設計にする

詳細はこれ以降の節で話していきますが、概要だけ述べておきます。

まず1つ目のセッションですが、多くのWebAuthnのサンプルを見たところ、ほぼセッションで実装していましたが、
フロントエンドとバックエンドが疎結合となっているSPA（またはモバイルアプリ）の場合、トークンのほうが実装しやすいので、
セッションを使わない実装をしています。つまりCookieを使用しません。

2つめのエンドポイント設計ですが、フロントエンドとバックエンドが分離しやすい構成なので、
それらを合わせるエンドポイント（URI）設計が重要になってきます。
フロントを自作してバックはOSSを使う。逆にバックは自作してフロントにOSSを使う。
のように、それぞれで実装を入れ替えても動くような仕組みで作りたいなと考えます。
こちらに関してはFIDO AllianceがServer Requirementsという文章を公開しているので、その文章の提案（仕様ではない）に沿って実装します。

== WebAuthnのフローの解説

書こうと思いましたが、@shiroica 氏がより詳しい流れを書いているので詳しく書くのをやめました。

== Server Requirements

FIDO Allianceは「Server Requirements and Transport Binding Profile」@<fn>{server_requirements}という文書を公開しており、
この中にAPIのパスやリクエストの形式まで詳細に書いてあるので、こちらに沿って実装していくと取り回しがしやすくなると思います。

//footnote[server_requirements][Server Requirements and Transport Binding Profile： @<href>{https://fidoalliance.org/specs/fido-v2.0-rd-20180702/fido-server-v2.0-rd-20180702.html} ]


この文書は「This document contains a non-normative, proposed REST API for FIDO2 servers.」と記載されているので、仕様というより提案のようです。

FIDOには「Conformance Self‐Validation Testing」@<fn>{conformance_testing}と呼ばれるテストツールが存在して、仕様に準拠してるかを網羅的にテストしてくれます。サーバーを開発するときはこのテストツールに沿って開発をすると良いでしょう。

//footnote[conformance_testing][Conformance Self‐Validation Testing： @<href>{https://fidoalliance.org/certification/functional-certification/conformance/} ]

この提案書は、主にこのConformance Toolに向けた文書らしいのですが、2019/04段階ではReview Draftとなっています。多少情報が古くて最新仕様に追従されていないので、本来のリクエスト/レスポンスの形式を確認するときは、大本の仕様を確認してください。"requireResidentKey" が "residentKey" になっていたりしました。

このテストツールはまだベータ版です。エラーメッセージが間違っていたりとまだまだバグがあるようですが、issueの管理がGitHubで行われている@<fn>{conformance_tools_issue}ので、問題を見つけたらissueを書いていきましょう。
先日もツールに関するバグを見つけて起票したところ、FIDOエヴァンジェリストのYuriyが見つけてサッとfixしてくれました。

//footnote[conformance_tools_issue][Conformance Tools Issues： @<href>{https://github.com/fido-alliance/conformance-tools-issues} ]

== コントローラ設計

Railsでコントローラを実装する時は、DHH流の書き方で実装したくなります。
DHH流の書き方を知らない場合は「DHHはどのようにRailsのコントローラを書くのか」を読んでください。


//footnote[dhh_controller][DHHはどのようにRailsのコントローラを書くのか： @<href>{https://postd.cc/how-dhh-organizes-his-rails-controllers/} ]

私が見てきた多くのRailsアプリケーションでは、認証処理をセッションを作成すると読み替えて、
SessionsController#create で行っていました。これは/sessionsに対するPOSTリクエストになります。

この流儀とServer Requirementsに沿って設計すると、サーバーサイド側のコントローラは次のようになります。

 * Attestation::OptionsController
 * Attestation::ResultController
 * Assertion::OptionsController
 * Assertion::ResultController

FIDOの文脈では「Attestation」は鍵の登録、「Assertion」は鍵を使ったログイン処理として使われます。
routes.rbは次のような定義となります。

//listnum[routes][routes.rb]{
Rails.application.routes.draw do
  resource :attestation, only: [], module: 'attestation' do
    resource :options, only: [:create]
    resource :result, only: [:create]
  end

  resource :assertion, only: [], module: 'assertion' do
    resource :options, only: [:create]
    resource :result, only: [:create]
  end
end
//}

全てcreateメソッドになります。面白いですね。
このエンドポイントに対して、フロントエンドにも同様の定義を書いてAxiosからコールしてます。

//listnum[frontend_endpoints_definition][Frontend Endpoints Definition]{
const WEBAUTHN_API = {
  // ユーザー登録
  ATTESTATION: {
    OPTIONS: '/attestation/options',
    RESULT: '/attestation/result',
  },
  // ログイン
  ASSERTION: {
    OPTIONS: '/assertion/options',
    RESULT: '/assertion/result',
  }
};
//}

== ユーティリティ

WebAuthnの一連のフローの内に、バイナリと文字列の相互変換処理が何度も実行されます。
しかもフロントエンド、サーバーサイド両方で。
変換ユーティリティををコントローラー等に実装しておくとよいです。
何回も呼び出していると非常に混乱してきます。

//listnum[frontend_utility][Frontend Utility Methods]{
  const encoder = {
    binToStr: (bin) => btoa(new Uint8Array(bin).reduce(
     (s, byte) => s + String.fromCharCode(byte), '')),
    strToBin: (str) => Uint8Array.from(atob(str), c => c.charCodeAt(0))
  };
//}

サーバーサイドの場合はベースコントローラーに実装しておきます。

//listnum[backend_utility][Backend Utility Methods]{
class ApplicationController < ActionController::API
  def str_to_bin(str)
    Base64.urlsafe_decode64(str)
  end

  def bin_to_str(bin)
    Base64.urlsafe_encode64(bin, padding: false)
  end

  def webauthn_origin
    return ENV['WEBAUTHN_ORIGIN'] if Rails.env.production?

    request.headers['origin']
  end

  def webauthn_rpid
    URI.parse(webauthn_origin).host
  end
end
//}

== 登録 Attestation

まず登録時の処理を実装してみました。
整理すると次のような処理流れになります。

 * チャレンジを含むオプションの生成
 * 一部パラメーターの変換
 * WebAuthnAPI（navigator.credentials.create) の呼び出し
 * WebAuthnAPIの呼び出し結果をサーバーへ送信して鍵検証・登録

では、オプション生成の全体像を見ていきます。

//listnum[attestation_option][controllers/attestation/options_controller.rb]{
module Attestation
  class OptionsController < ApplicationController
    def create
      credential_options = build_credential_options

      payload = {
        username: params[:username],
        exp: Time.current.to_i + credential_options[:timeout] / 1000 + 5,
        nonce: bin_to_str(SecureRandom.random_bytes(32))
      }
      credential_options[:challenge] = Base64.urlsafe_encode64(
        ::JWT.encode(payload, ENV['WEBAUTHN_SECRET'], 'HS256')
      )

      user = User.find_by(username: params[:username])

      if user.present? && !user.registered
        user.update!(nonce: payload[:nonce])
      else
        User.create!(username: params[:username], nonce: payload[:nonce])
      end

      render json: credential_options.merge(status: 'ok', errorMessage: '')
    rescue StandardError => e
      render json: { status: 'failed', errorMessage: e.message }, status: :unprocessable_entity
    end

    private

    def build_credential_options
      credential_options = ::WebAuthn.credential_creation_options
      credential_options[:user][:id] = bin_to_str(params[:username])
      credential_options[:user][:name] = params[:username]
      credential_options[:user][:displayName] = params[:displayName]
      credential_options[:timeout] = 20_000
      credential_options[:rp][:id] = webauthn_rpid
      credential_options[:rp][:name] = webauthn_rpid
      credential_options[:attestation] = params[:attestation] || 'none'

      if params[:authenticatorSelection]
        credential_options[:authenticatorSelection] = params[:authenticatorSelection].slice(
          :requireResidentKey, :authenticatorAttachment, :userVerification
        )
      end

      credential_options
    end
  end
end
//}

まずはオプション生成部です。
検証部分はWebAuthn Gem@<fn>{webauthn_gem}にゴリゴリに依存しているのですが、
WebAuthn Gemのメソッドで作成できるオプションでは不足しているので一部上書きします。

//footnote[webauthn_gem][WebAuthn ruby library： @<href>{https://github.com/cedarcode/webauthn-ruby} ]

//listnum[attestation_option_build_option][Attestation/オプションの初期化]{
def create
  credential_options = build_credential_options
  ...
end

private

def build_credential_options
  credential_options = ::WebAuthn.credential_creation_options
  credential_options[:user][:id] = bin_to_str(params[:username])
  credential_options[:user][:name] = params[:username]
  credential_options[:user][:displayName] = params[:displayName]
  credential_options[:timeout] = 20_000
  credential_options[:rp][:id] = webauthn_rpid
  credential_options[:rp][:name] = webauthn_rpid
  credential_options[:attestation] = params[:attestation] || 'none'

  if params[:authenticatorSelection]
    credential_options[:authenticatorSelection] = params[:authenticatorSelection].slice(
      :requireResidentKey, :authenticatorAttachment, :userVerification
    )
  end

  credential_options
end
//}

次にセッションレス実装の肝の部分です。セッションを使用する代わりに、usernameを埋め込んだJWTをチャレンジとして使用します。
JWTの中にnonceを埋め込んでリプレイアタック対策とします。
JWTの有効期限は、WebAuthn APIのオプションのタイムアウトを基準として、少しの余裕をもたせています。
これは検証時にleewayオプションを渡してもいいかもしれないです。

//listnum[attestation_option_build_jwt][Attestation/チャレンジ（JWT）生成]{
payload = {
  username: params[:username],
  exp: Time.current.to_i + credential_options[:timeout] / 1000 + 5,
  nonce: bin_to_str(SecureRandom.random_bytes(32))
}
credential_options[:challenge] = Base64.urlsafe_encode64(
  ::JWT.encode(payload, ENV['WEBAUTHN_SECRET'], 'HS256')
)
//}

余談ですが、WebAuthn Gemはデフォルトで以下のようなチャレンジ生成をしていました。

//listnum[webauthn_gem_challenge][WebAuthn Gemデフォルトのチャレンジ生成]{
def self.challenge
  SecureRandom.random_bytes(32)
end
//}

次はユーザーの操作です。
初回リクエストの場合はユーザーが存在しないので作成し、リトライしたケースは既にユーザーが存在するため、更新となります。
ユーザーにはnonceを紐づけておきます。
ここで鍵の登録が終わっているユーザーを弾こうかと思ったのですが、複数鍵を登録ケースに対応できないのでやめました。

//listnum[attestation_option_user][Attestation/ユーザーの作成・更新]{
user = User.find_by(username: params[:username])

if user.present?
  user.update!(nonce: payload[:nonce])
else
  User.create!(username: params[:username], nonce: payload[:nonce])
end
//}

今回の実装だとnonceがMySQLの中に入っているのですが、NoSQLで管理してもいいかもと思いました。

次にこのattestation/options APIを呼び出すフロントエンド側の実装をざっと見ていきます。

//listnum[attestation_front][Attestation/フロントエンド側の実装]{
const attestation = async (username) => {
  // WebAuthn API用の呼び出し用のオプションの生成、チャレンジ生成
  const credentialOptions = await app.$axios.$post(
    WEBAUTHN_API.ATTESTATION.OPTIONS, {
    username: username,
    displayName: username,
    attestation: 'none'
  });

  if (credentialOptions) {
    // バイナリ変換
    credentialOptions["challenge"] = encoder.strToBin(credentialOptions["challenge"]);
    credentialOptions["user"]["id"] = encoder.strToBin(credentialOptions["user"]["id"]);

    // WebAuthentication APIの呼び出し
    const attestation = await navigator.credentials.create({"publicKey": credentialOptions});

    // API呼び出し結果をサーバーサイドへ送信
    const requestBody = {
      id: attestation.id,
      rawId: attestation.id,
      response: {
        clientDataJSON: encoder.binToStr(attestation.response.clientDataJSON),
        attestationObject: encoder.binToStr(attestation.response.attestationObject)
      },
      type: 'public-key'
    };
    const registResponse = await app.$axios.$post(WEBAUTHN_API.ATTESTATION.RESULT, requestBody);
    if (registResponse) {
      console.log(`${username} registration succeeded.`);
    }
  }
};
//}

フロントエンド側は非常にシンプルでサーバーサイドで生成したオプションを使用してWebAuthn APIをコールし、結果をサーバーサイドに返しているだけです。APIをコールするために一部の項目は文字列からバイナリへ変換する必要はありますが。

WebAuthn API(create/get)に渡すオプションをフロントエンドで生成してる例をちらほら見かけますが、フロントエンドから最低限必要な項目を受け取って、サーバーサイドでチャレンジと一緒に生成しましょう。

最後に鍵の登録部分です。

//listnum[attestation_result][Attestation/APIレスポンスの検証]{
module Attestation
  class ResultsController < ApplicationController
    def create
      auth_response = build_auth_response

      jwt = Base64.urlsafe_decode64(auth_response.client_data.challenge)
      payload = ::JWT.decode(jwt, ENV['WEBAUTHN_SECRET'], true, algorithm: 'HS256')

      user = User.find_by!(username: payload[0]['username'])

      raise WebAuthn::VerificationError if user.nonce != payload[0]['nonce']

      user.update!(nonce: '')

      raise WebAuthn::VerificationError unless auth_response.verify(jwt, webauthn_origin)

      ActiveRecord::Base.transaction do
        credential = user.credentials.find_or_initialize_by(
          cred_id: Base64.strict_encode64(auth_response.credential.id)
        )
        credential.update!(
          public_key: Base64.strict_encode64(auth_response.credential.public_key)
        )
        user.update!(registered: true)
      end

      render json: { status: 'ok', errorMessage: '' }, status: :ok
    rescue StandardError => e
      render json: { status: 'failed', errorMessage: e.message }, status: :unprocessable_entity
    end

    private

    def build_auth_response
      WebAuthn::AuthenticatorAttestationResponse.new(
        attestation_object: str_to_bin(params[:response][:attestationObject]),
        client_data_json: str_to_bin(params[:response][:clientDataJSON])
      )
    end
  end
end
//}

WebAuthn Gemを使ってパラメーターから専用のインスタンスを作成します。

//listnum[attestation_result_response][Attestation/オブジェクトの組み立て]{
def create
  auth_response = build_auth_response
  ...
end

private

def build_auth_response
  WebAuthn::AuthenticatorAttestationResponse.new(
    attestation_object: str_to_bin(params[:response][:attestationObject]),
    client_data_json: str_to_bin(params[:response][:clientDataJSON])
  )
end
//}

JWTからusernameを取り出します。


//listnum[attestation_result_jwt][Attestation/JWTからユーザーの読み出し]{
jwt = Base64.urlsafe_decode64(auth_response.client_data.challenge)
payload = ::JWT.decode(jwt, ENV['WEBAUTHN_SECRET'], true, algorithm: 'HS256')

user = User.find_by!(username: payload[0]['username'])
//}

WebAuthn GemのverifyだけではJWT内部に含まれているnonceのチェックが当然できていないので、
nonceは個別にチェックする必要があります。チェックしたらnonceは消します。

//listnum[attestation_result_verify][Attestation/リクエストの検証]{
raise WebAuthn::VerificationError if user.nonce != payload[0]['nonce']

user.update!(nonce: '')

raise WebAuthn::VerificationError unless auth_response.verify(jwt, webauthn_origin)
//}

最後にcredentialIDを登録して完了です。

//listnum[attestation_result_save_credential][Attestation/クレデンシャルの保存]{
ActiveRecord::Base.transaction do
  credential = user.credentials.find_or_initialize_by(
    cred_id: Base64.strict_encode64(auth_response.credential.id)
  )
  credential.update!(
    public_key: Base64.strict_encode64(auth_response.credential.public_key)
  )
  user.update!(registered: true)
end
//}

== 認証 Assertion

次の鍵の検証（ログイン処理）の話です。
フローとしては鍵の登録とほぼ同じなので、開発が省略気味です。

//listnum[assertion_option][Assertion/鍵の検証]{
module Assertion
  class OptionsController < ApplicationController
    def create
      user = User.find_by!(username: params[:username])
      raise StandardError unless user.registered

      credential_options = build_credential_options
      credential_options[:allowCredentials] = user.credentials.map do |credential|
        { id: credential.cred_id, type: 'public-key' }
      end

      payload = {
        username: params[:username],
        exp: Time.current.to_i + credential_options[:timeout] / 1000 + 5,
        nonce: bin_to_str(SecureRandom.random_bytes(32))
      }
      credential_options[:challenge] = Base64.urlsafe_encode64(::JWT.encode(payload, ENV['WEBAUTHN_SECRET'], 'HS256'))

      user.update!(nonce: payload[:nonce])

      render json: credential_options.merge(status: 'ok', errorMessage: '')
    rescue StandardError => e
      render json: { status: 'failed', errorMessage: e.message }, status: :unprocessable_entity
    end

    private

    def build_credential_options
      credential_options = WebAuthn.credential_request_options
      credential_options[:timeout] = 20_000
      credential_options[:userVerification] = params[:userVerification] || 'required'
      credential_options[:rpId] = webauthn_rpid
      credential_options
    end
  end
end
//}

検証時のオプションは非常に少なくて、許可されたcredentialIDを渡してあげます。

//listnum[assertion_option_credential][Assertion/許可するクレデンシャル]{
credential_options[:allowCredentials] = user.credentials.map do |credential|
  { id: credential.cred_id, type: 'public-key' }
end
//}

フロントエンドの処理もAttestationと基本の流れは同じですね。


//listnum[assertion_option_credential][Assertion/フロントエンド側の実装]{
const assertion = async (username) => {
  const credentialOptions = await app.$axios.$post(WEBAUTHN_API.ASSERTION.OPTIONS, {
    username: username,
    userVerification: 'required'
  });

  if (credentialOptions) {
    credentialOptions["challenge"] = encoder.strToBin(credentialOptions["challenge"]);
    credentialOptions["allowCredentials"].forEach(function (cred) {
      cred["id"] = encoder.strToBin(cred["id"]);
    });

    const assertion = await navigator.credentials.get({"publicKey": credentialOptions});
    const requestBody = {
      id: encoder.binToStr(assertion.rawId),
      rawId: encoder.binToStr(assertion.rawId),
      response: {
        clientDataJSON: encoder.binToStr(assertion.response.clientDataJSON),
        signature: encoder.binToStr(assertion.response.signature),
        userHandle: encoder.binToStr(assertion.response.userHandle),
        authenticatorData: encoder.binToStr(assertion.response.authenticatorData)
      }
    };

    const sessionCreateResponse = await app.$axios.post(WEBAUTHN_API.ASSERTION.RESULT, requestBody);
    if (sessionCreateResponse) {
      console.log(`${username} login succeeded.`);
    }
  }
};
//}

assertion/result に関しても、attestationとほぼ同様の処理です。
異なるのは検証時に許可されたcredentialIDを渡しているぐらいです。

//listnum[assertion_result][Assertion/APIレスポンスの検証]{
module Assertion
  class ResultsController < ApplicationController
    def create
      auth_response = build_auth_response

      jwt = Base64.urlsafe_decode64(auth_response.client_data.challenge)
      payload = ::JWT.decode(jwt, ENV['WEBAUTHN_SECRET'], true, algorithm: 'HS256')

      user = User.find_by!(username: payload[0]['username'])

      allowed_credentials = user.credentials.map do |credential|
        {
          id: str_to_bin(credential.cred_id),
          public_key: str_to_bin(credential.public_key)
        }
      end

      raise WebAuthn::VerificationError if user.nonce != payload[0]['nonce']

      user.update!(nonce: '')

      render json: { status: 'failed' }, status: :forbidden unless auth_response.verify(
        jwt,
        webauthn_origin,
        allowed_credentials: allowed_credentials
      )

      render json: { status: 'ok', errorMessage: '' }, status: :ok
    rescue StandardError => e
      render json: { status: 'failed', errorMessage: e.message }, status: :forbidden
    end

    def build_auth_response
      WebAuthn::AuthenticatorAssertionResponse.new(
        credential_id: str_to_bin(params[:id]),
        client_data_json: str_to_bin(params[:response][:clientDataJSON]),
        authenticator_data: str_to_bin(params[:response][:authenticatorData]),
        signature: str_to_bin(params[:response][:signature])
      )
    end
  end
end
//}

認証して満足してしまったのですが、
この認証の後に本セッション用のトークンを発行したりセッションを発行すれば認証処理は完了となります。

== 余談1: WebAuthnの自動テスト

私はWebアプリケーションを作る時にテストをどうするのかをよく考えます。
テストがあるとプログラム改修の心理的負荷が下がるので、テストは重要です。
体感的にはプロダクションコードよりもテストを書いている時間のほうが長いぐらいです。

サーバーサイドのテストはConformance Toolもありますし、
実際のリクエストを利用すれば自動テストによるデグレチェックは容易です。

SPA時代のフロントエンドのテストは、テストフレームワークは色々と選択肢がありますが、
DOMのレンダリングにjsdomを使うケースが多いです。
こちらはnagivator.credentialsには対応してないので、モックするなりしてテストする必要があるなと考えます。

E2E（エンドツーエンド）なテストの場合は、chromedriver を使ったテストが多いのではないでしょうか。
こちらの場合実際のChromeを自動操作することが可能にはなるのですが、CIで動作させる場合は当然ながらFIDOキーが準備されていないので、
何かしら仮想的なAuthenticatorがあるといいなあと思いました。

== 余談2: webauthn.js プラグイン

Nuxt.jsの場合はプラグインとして切り出しておくと非常に便利です。
なおAPIの特性上SSR（Server-Side-Rendering）で動かないので注意してください。

//listnum[plugins_webauthn][plugins/webauthn.js]{
export default ({ app }, inject) => {
  const attestation = async (username) => { 略 };
  const assertion = async (username) => { 略 };
  const encoder = { 略 };

  inject('webauthn', {
    attestation,
    assertion,
    encoder
  });
};
//}

上記のようにinjectしておくと、this.$webauthn で呼び出せます。
コンポーネント側スッキリです。

//listnum[pages_index_vue][pages/index.vue]{
methods: {
  async attestation() {
    try {
      await this.$webauthn.attestation(this.username);
    } catch (e) {
      console.error(e);
    }
  },
  async assertion() {
    try {
      await this.$webauthn.assertion(this.username)
    } catch (e) {
      console.error(e);
    }
  }
}
//}

こちらはnpmで公開したら誰か使ってくれるかも？

== まとめ

SPAでセッションレスでWebAuthnを試してみました。
アーキテクチャに合わせた実装例がたくさん世に出てくると検討コストも下がってくると思うので、みんなもっとオープンにしていきましょう。

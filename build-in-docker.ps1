function pwd_as_linux {
    "/$((pwd).Drive.Name.ToLowerInvariant())/$((pwd).Path.Replace('\', '/').Substring(3))"
}

docker run -t --rm -v "$(pwd_as_linux):/book_tmp" vvakame/review:2.4 /bin/bash -ci "mkdir /book/ && cp -aR /book_tmp/. /book/ && cd /book && npm install && git submodule update -i && npm run pdf && cp ./articles/WebAuthn-Tech-Book2.pdf /book_tmp/"

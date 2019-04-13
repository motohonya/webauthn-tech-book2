function pwd_as_linux {
    "/$((pwd).Drive.Name.ToLowerInvariant())/$((pwd).Path.Replace('\', '/').Substring(3))"
}

docker run -t --rm -v "$(pwd_as_linux):/book" vvakame/review:2.4 /bin/bash -ci "/book/build-in-vm.sh"

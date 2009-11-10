File with("urls.txt") contents split("\n") foreach(u, URL with("http://localhost:8080" .. u) fetch println)

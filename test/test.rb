#!/usr/bin/ruby

require 'net/http'

server = URI.parse(ARGV[0])

http = Net::HTTP.new(server.host, server.port)

puts "# -- request new session"
req = Net::HTTP::Get.new('/')
res = http.request(req)

if res.code != '200'
  puts "failure with code #{res.code}"
  exit 1
end

all_cookies = res.get_fields('set-cookie')
cookies_array = Array.new
all_cookies.each { | cookie |
  cookies_array.push(cookie.split('; ')[0])
}
cookies = cookies_array.join('; ')
puts "Cookies: #{cookies}"
puts res.body

puts "# -- login"
req = Net::HTTP::Post.new('/', { 'Cookie' => cookies })
req.set_form_data({"user" => "admin", "password" => "admin"})
res = http.request(req)
puts res.body

if ! res.body.include? "Willkommen"
  puts "login failed"
  exit 1
end

puts "# -- query members"
req = Net::HTTP::Get.new('/member', { 'Cookie' => cookies })
req.set_form_data({})
res = http.request(req)
puts res.body

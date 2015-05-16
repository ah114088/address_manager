#!/usr/bin/ruby

require 'net/http'

def login http, cookies, password
	puts "# -- login"
	req = Net::HTTP::Post.new('/', { 'Cookie' => cookies })
	req.set_form_data({"user" => "admin", "password" => password})
	res = http.request(req)
	# puts res.body

	if ! res.body.include? "Willkommen"
		puts "login failed"
		exit 1
	end
end

def logout http, cookies
	puts "# -- logout"
	req = Net::HTTP::Post.new('/logout', { 'Cookie' => cookies })
	req.set_form_data({})
	res = http.request(req)
	# puts res.body
end

def passwd http, cookies, newpassword
	puts "# -- passwd"
	req = Net::HTTP::Post.new('/chpass', { 'Cookie' => cookies })
	req.set_form_data({"newpassword" => newpassword})
	res = http.request(req)
	# puts res.body
end

def members http, cookies
	puts "# -- query members"
	req = Net::HTTP::Get.new('/member', { 'Cookie' => cookies })
	req.set_form_data({})
	res = http.request(req)
	# puts res.body
end

if ARGV.length != 1
	puts "usage: test.rb <URL>"
	exit 1
end

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
# puts res.body

login(http, cookies, "admin")
# members(http, cookies)
logout(http, cookies)

login(http, cookies, "admin")
passwd(http, cookies, "geheim")
logout(http, cookies)

login(http, cookies, "geheim")
logout(http, cookies)

login(http, cookies, "geheim")
passwd(http, cookies, "admin")
logout(http, cookies)

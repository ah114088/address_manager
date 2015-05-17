#!/usr/bin/ruby

require 'net/http'

def login http, cookies, user, password
	puts "# -- login"
	req = Net::HTTP::Post.new('/', { 'Cookie' => cookies })
	req.set_form_data({"user" => user, "password" => password})
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

def newuser http, cookies, username, password, fid
	puts "# -- new user"
	req = Net::HTTP::Post.new('/newuser', { 'Cookie' => cookies })
	req.set_form_data({"username" => username, "password" => password, "fid" => fid})
	res = http.request(req)
	# puts res.body
end

def newmember http, cookies, first, second, street, house, zip, city, email, fid
	puts "# -- new member"
	req = Net::HTTP::Post.new('/newmember', { 'Cookie' => cookies })
	req.set_form_data({"first" => first, "second" => second, "street" => street, "house" => house, 
			"zip" => zip, "city" => city, "email" => email, "fid" => fid})
	res = http.request(req)
	# puts res.body
end

def newformation http, cookies, name, fid
	puts "# -- new formation"
	req = Net::HTTP::Post.new('/newformation', { 'Cookie' => cookies })
	req.set_form_data({"name" => name, "fid" => fid})
	res = http.request(req)
	# puts res.body
end

def newformation_bug http, cookies, name, fid
	puts "# -- new formation"
	req = Net::HTTP::Post.new('/newformation', { 'Cookie' => cookies })
	req.set_form_data({"bug" => "bug", "name" => name, "fid" => fid})
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

login(http, cookies, "admin", "admin")
# members(http, cookies)
logout(http, cookies)

login(http, cookies, "admin", "admin")
passwd(http, cookies, "geheim")
logout(http, cookies)

login(http, cookies, "admin", "geheim")
logout(http, cookies)

login(http, cookies, "admin", "geheim")
passwd(http, cookies, "admin")
logout(http, cookies)

login(http, cookies, "admin", "admin")
newuser(http, cookies, "mfr", "mfr", "0")
logout(http, cookies)

login(http, cookies, "mfr", "mfr")
logout(http, cookies)

login(http, cookies, "admin", "admin")
newmember(http, cookies, "Michaela", "Muster", "Kurfürstendamm", "11", "10707", "Berlin", "mm@gmx.de", 0)
newformation(http, cookies, "Hongkong", "0")
logout(http, cookies)

# these cases should be tested as well
# newformation(http, cookies, "Hongkong", "00000000") # shoult raise an error
# newformation_bug(http, cookies, name, fid) # shoult raise an error

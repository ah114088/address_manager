#!/usr/bin/ruby

require 'net/http'

class Session
	def initialize uri
		server = URI.parse(uri)
		@http = Net::HTTP.new(server.host, server.port)

		puts "# -- request new session"
		req = Net::HTTP::Get.new('/')
		res = @http.request(req)

		if res.code != '200'
			puts "failure with code #{res.code}"
			exit 1
		end
		all_cookies = res.get_fields('set-cookie')
		cookies_array = Array.new
		all_cookies.each { | cookie |
			cookies_array.push(cookie.split('; ')[0])
		}
		@cookies = cookies_array.join('; ')
		puts "Cookies: #{@cookies}"
	end

	def login user, password
		puts "# -- login"
		req = Net::HTTP::Post.new('/', { 'Cookie' => @cookies })
		req.set_form_data({"user" => user, "password" => password})
		res = @http.request(req)
		if ! res.body.include? "Willkommen"
			puts "login failed"
			exit 1
		end
	end
	def logout
		puts "# -- logout"
		req = Net::HTTP::Post.new('/logout', { 'Cookie' => @cookies })
		res = @http.request(req)
	end

	def passwd newpassword
		puts "# -- passwd"
		req = Net::HTTP::Post.new('/chpass', { 'Cookie' => @cookies })
		req.set_form_data({"newpassword" => newpassword})
		res = @http.request(req)
	end
	def members
		puts "# -- query members"
		req = Net::HTTP::Get.new('/member', { 'Cookie' => @cookies })
		res = @http.request(req)
		# puts res.body
	end
	def newuser username, password, fid
		puts "# -- new user"
		req = Net::HTTP::Post.new('/newuser', { 'Cookie' => @cookies })
		req.set_form_data({"username" => username, "password" => password, "fid" => fid})
		res = @http.request(req)
	end
	def newmember first, second, street, house, zip, city, email, fid
		puts "# -- new member"
		req = Net::HTTP::Post.new('/newmember', { 'Cookie' => @cookies })
		req.set_form_data({"first" => first, "second" => second, "street" => street, "house" => house, 
				"zip" => zip, "city" => city, "email" => email, "fid" => fid})
		res = @http.request(req)
	end

	def newformation name, fid
		puts "# -- new formation"
		req = Net::HTTP::Post.new('/newformation', { 'Cookie' => @cookies })
		req.set_form_data({"name" => name, "fid" => fid})
		res = @http.request(req)
	end
	def newformation_bug name, fid
		puts "# -- new formation"
		req = Net::HTTP::Post.new('/newformation', { 'Cookie' => @cookies })
		req.set_form_data({"bug" => "bug", "name" => name, "fid" => fid})
		res = @http.request(req)
	end
end


# --- main ---

if ARGV.length != 1
	puts "usage: test.rb <URL>"
	exit 1
end

session = Session.new(ARGV[0])

session.login "admin", "admin"
session.members
session.logout

session.login "admin", "admin"
session.passwd "geheim"
session.logout

session.login "admin", "geheim"
session.logout

session.login "admin", "geheim"
session.passwd "admin"
session.logout

session.login "admin", "admin"
session.newuser "mfr", "mfr", "0"
session.logout

session.login "mfr", "mfr"
session.logout

session.login "admin", "admin"
session.newmember "Michaela", "Muster", "Kurfürstendamm", "11", "10707", "Berlin", "mm@gmx.de", 0
session.newformation "Hongkong", "0"
session.logout

# these cases should be tested as well
# newformation(http, cookies, "Hongkong", "00000000") # shoult raise an error
# newformation_bug(http, cookies, name, fid) # shoult raise an error

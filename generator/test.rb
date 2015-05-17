#!/usr/bin/ruby

require 'session'

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

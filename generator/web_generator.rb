#!/usr/bin/ruby
# encoding: utf-8

require 'http_session'
require 'person_generator'

def usage
	puts "generator.rb [-select <formation>] <number_of_members> <zip_code_file> <firstname_file> <secondname_file> <URI>"
  exit 1
end

# ---- main ----

usage if ARGV.length < 2

formation = nil
if ARGV[0] == "-select"
	ARGV.shift
	formation = ARGV.shift
end
usage if ARGV.length != 5

g = PersonGenerator.new(ARGV[1], ARGV[2], ARGV[3], formation)

session = Session.new ARGV[4]
session.login "admin", "admin"

g.formation.each { |f|
	puts "adding #{f.name}\t#{f.sform}"
	session.newformation f.name, f.sform
}

ARGV[0].to_i.times {
	p = g.get_person
	p.print
	session.newmember p.first, p.second, p.street, p.house, p.zip, p.city, p.email, p.findex
}

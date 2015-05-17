#!/usr/bin/ruby
# encoding: utf-8

require 'person_generator'

def usage
	puts "generator.rb [-select <formation>] <number_of_members> <zip_code_file> <firstname_file> <secondname_file> <person_file> <formation_file>"
  exit 1
end
# ---- main ----

usage if ARGV.length < 2

formation = nil
if ARGV[0] == "-select"
	ARGV.shift
	formation = ARGV.shift
end
usage if ARGV.length != 6

g = PersonGenerator.new(ARGV[1], ARGV[2], ARGV[3], formation)

File.open(ARGV[4], "w") { |file|
	ARGV[0].to_i.times {
		p = g.get_person
		p.print file
	}
}

File.open(ARGV[5], "w") { |file|
	i = 0
	g.formation.each { |f|
		file.puts "#{i}\t#{f.name}\t#{f.sform}"
		i += 1
	}
}

exit 0


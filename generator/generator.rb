#!/usr/bin/ruby

class Locality
	attr_accessor :zip, :name, :country, :province, :circle
	def initialize zip, name, country, province, circle
		@zip = zip
		@name = name
		@country = country
		@province = province
		@circle = circle
	end
	def occur
		1
	end
	def print
		puts "#{@zip} #{@name}"
	end
end

class LocalityList < Array
	def initialize file, formation
		File.open(file).each { |l|
			s = l.split("\t")
			next if s[7].length == 0
			if formation.nil? || s[3] == formation || s[5] == formation || s[7] == formation 
				self << Locality.new(s[1], s[2], s[3], s[5], s[7])
			end
		}
	end
	def sum
		self.length
	end
end

class Name
	attr_accessor :name, :occur
	def initialize name, occur
		@name = name
		@occur = occur
	end
end

class NameList < Array
	attr_reader :sum
	def initialize file
		File.open(file).each { |l|
			s = l.split("\t")
			self << Name.new(s[0], s[1].to_i)
		}
		@sum = 0	
		self.each { |n| @sum += n.occur }
	end
end

class MailProviderList < Array
	def initialize
	[ "gmail.com", "gmx.de", "afdbayern.de", "alternativefuer.de", "t-online.de" ].each { |domain|
			self << Name.new(domain, 1)
	}
	end
	def sum
		self.length
	end
end

class Formation
	attr_accessor :name, :sform
	def initialize name, sform = -1
		@name = name
		@sform = sform
	end
end

class FormationList < Array
	def initialize locations
		self << Formation.new("Bund")
		locations.each { |l|
			i = self.find_index { |item| item.name == l.country }
			next if ! i.nil?
			self << Formation.new(l.country, 0) 
		}
		locations.each { |l|
			next if l.province.length == 0
			i = self.find_index { |item| item.name == l.province }	
			next if ! i.nil?
			country_index = self.find_index { |item| item.name == l.country }
			self << Formation.new(l.province, country_index) 
		}
		locations.each { |l|
			i = self.find_index { |item| item.name == l.circle }
			next if ! i.nil?
			if l.province.length == 0
				country_index = self.find_index { |item| item.name == l.country }
				self << Formation.new(l.circle, country_index) 
			else
				province_index = self.find_index { |item| item.name == l.province }
				self << Formation.new(l.circle, province_index) 
			end
		}
	end
end

class Person
	attr_accessor :first, :second, :street, :house, :zip, :city, :country, :email, :findex
	def print o = STDOUT
  	o.puts "#{@first}\t#{@second}\t#{@street}\t#{@house}\t#{@zip}\t#{@city}\t#{@country}\t#{@email}\t#{@findex}"
	end
end

class PersonGenerator
	attr_reader :zip_list, :formation
	def initialize zip_file, first_file, second_file, formation
		@zip_list = LocalityList.new zip_file, formation
		@first = NameList.new first_file
		@second = NameList.new second_file
		@mail_provider = MailProviderList.new
		@formation = FormationList.new @zip_list
	end
	def generate list
		number = rand(list.sum - 1) + 1
		# puts "number: #{number}"
		count = 0
		list.each { |n|
			count += n.occur
			return n if count > number
		}
		STDERR.puts "huh?"
	end
	def convert_umlaut s
#		s.sub! 'ä', 'ae'
#		s.sub! 'ö', 'oe'
#		s.sub! 'ü', 'ue'
#		s.sub! 'ß', 'ss'
		s
	end
	def get_person
		p = Person.new
		first = generate(@first).name
		second = generate(@second).name
		p.first = first
		p.second = second
		p.street = "#{generate(@first).name}-#{generate(@second).name}-Str."	
		p.house = rand(100)+1

		locality = generate(@zip_list)
		p.zip = locality.zip
		p.city = locality.name
		p.country = locality.country

		p.email = "#{convert_umlaut(first).downcase}.#{convert_umlaut(second).downcase}@#{generate(@mail_provider).name}"

		p.findex = @formation.find_index	{ |item| locality.circle == item.name }
		p
	end
end

# ------------------------

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

# ----------

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

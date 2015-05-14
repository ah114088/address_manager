#!/usr/bin/ruby

class Locality
	attr_accessor :zip, :name, :country
	def initialize zip, name, country
		@zip = zip
		@name = name
		@country = country
	end
	def occur
		1
	end
	def print
		puts "#{@zip} #{@name}"
	end
end

class LocalityList < Array
	def initialize file
		File.open(file).each { |l|
			s = l.split("\t")
			self << Locality.new(s[1], s[2], s[3])
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

class Person
	attr_accessor :first, :second, :street, :house, :zip, :city, :country, :email
	def print
  	puts "#{@first}\t#{@second}\t#{@street}\t#{@house}\t#{@zip}\t#{@city}\t#{@country}\t#{@email}"
	end
end

class PersonGenerator
	def initialize zip_file, first_file, second_file
		@zip_list = LocalityList.new zip_file
		@first = NameList.new first_file
		@second = NameList.new second_file
		@mail_provider = MailProviderList.new
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
		p
	end
end

# ---- main ----

if ARGV.length < 4 
	puts "generator.rb <number_of_members> <zip_code_file> <firstname_file> <secondname_file>"
  exit 1
end

g = PersonGenerator.new(ARGV[1], ARGV[2], ARGV[3])

ARGV[0].to_i.times {
	p = g.get_person
  p.print
}

exit 0

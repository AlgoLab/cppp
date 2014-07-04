#!/usr/bin/env ruby

require 'optparse'
require 'optparse/time'
require 'ostruct'
require 'pp'

class OptparseExample
  def self.parse(args)
    # The options specified on the command line will be collected in *options*.
    # We set default values here.
    options = OpenStruct.new
    options.library = []
    options.inplace = false
    options.encoding = "utf8"
    options.transfer_type = :auto
    options.verbose = false

    opt_parser = OptionParser.new do |opts|
      opts.banner = "Usage: random-ppp.rb [options]"

      opts.separator ""
      opts.separator "Specific options:"


      # Mandatory argument.
      opts.on("-m", "--matrix MATRIX_FILENAME",
        "Input matrix") do |matrix|
        options.matrix = matrix
      end

      opts.on("-r", "--replace [NUMBER]",
        "Introduces NUMBER random constraints") do |num|
        options.mutations = num.to_i
      end


      # No argument, shows at tail.  This will print an options summary.
      # Try it and see!
      opts.on_tail("-h", "--help", "Show this message") do
        puts opts
        exit
      end
    end
    opt_parser.parse!(args)
    options
  end
end

options = OptparseExample.parse(ARGV)

matrix = File.readlines options.matrix
matrix[0] =~ /(\d+)\s+(\d+)/
n, m = $1.to_i, $2.to_i

while (options.mutations > 0)
  i = rand(n)
  j = rand(m)
  if matrix[i+2][2*j] == '0'
    matrix[i+2][2*j] = '3'
    options.mutations = options.mutations - 1
  end
end

puts matrix

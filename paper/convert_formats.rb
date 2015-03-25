#!/usr/bin/env ruby

require 'fileutils'
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
      opts.banner = "Usage: convert.rb [options]"

      opts.separator ""
      opts.separator "Specific options:"


    # Mandatory argument.
      opts.on("-m", "--matrix MATRIX_FILENAME",
              "Input matrix") do |matrix|
        options.matrix = matrix
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

matrix = (File.readlines options.matrix).map { |r| r.chomp }

if matrix[0] =~ /^\s*(\d+)\s+(\d+)\s*$/
  format = 1
  matrix[0] =~ /(\d+)\s+(\d+)/
  n, m = $1.to_i, $2.to_i
else
  n, m = matrix.size - 1, matrix[1].size
  format = 2
end

m1 = matrix.map { |r| r.gsub(' ', '') }

if (format == 1)
  p = m1
  p.delete_at(1)
  p[0] = "header #{n}-#{m}"
else
  p = m1.map { |r| r.gsub('', ' ') }
  p.insert(1, "")
  p[0] = "#{n} #{m}"
end
# puts "format = #{format}"
# puts "n = #{n}"
# puts "m = #{m}"
# # puts "********************"
# # puts matrix[0]
# # puts matrix
# # puts "********************"
# # puts m1[0]
# # puts m1
# puts "********************"
puts p

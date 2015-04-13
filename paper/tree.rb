#!/usr/bin/env ruby
# coding: utf-8

# Copyright 2015
# Gianluca Della Vedova <http://gianluca.dellavedova.org>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
require 'fileutils'
require 'optparse'
require 'optparse/time'
require 'ostruct'
require 'pp'
require 'matrix'
require 'set'


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

      # -p options means to generate the characters names for persistent perfect phylogeny
      # i.e. alternating positive and negative characters
      opts.on("-p", "--persistent") do
        options.persistent = true
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

class LabeledMatrix
  attr_reader :s_num, :c_num, :s_labels, :c_labels, :matrix
  # :matrix is the hash of columns, where the keys are the character names and each value
  # is a column that is represented as a string
  # The column ordering is the lexicographic order of the strings

  def pp
    puts @s_num
    puts @c_num
    puts #{"@matrix}"
    delimiter = @c_labels.map { |c| '_' }.join('')
    @c_labels[0].length.times do |i|
      puts '      ' + @c_labels.map { |c| c[i] }.join('')
    end
    puts "______#{delimiter}"
    @s_num.times do |i|
      puts "#{s_labels[i]}:#{@c_labels.map { |c| @matrix[c][i] }.join('')}"
    end
    puts "______#{delimiter}"
  end

  def s_label(num)
    return "s%04d" % num
  end

  def c_label(num)
    if @persistent
      sign = num % 1
      root = (num - sign) / 2 + 1
      if sign == 0
        return "C%04d+" % root
      else
        return "C%04d-" % root
      end
    else
      return "C%05d" % (num + 1)
    end
  end


  def initialize(arr, persistent_chars)
    @persistent = persistent_chars
    m = arr.map { |r| r.chomp }
    @c_labels = Array.new
    if m[0][0] == "\#"
      # The first line contains the optional list of characters, separated
      # by ,;
      header = m[0][1..-1]
      @c_labels = header.split(/[,;]/)
    end
    @c_num = m[0].length
    @s_num = m.length

    @matrix = Hash.new()
    m[0].split('').each_with_index { |x, idx| @matrix[c_label(idx)] = String.new("") }
    m.map do |row|
      row.chomp.split('').map.with_index { |x, idx| @matrix[c_label(idx)] << x }
    end

    @c_labels = @matrix.keys unless @c_labels.length > 0
    @s_labels = m.map.with_index { |x, i| s_label(i) }
    @s_chars  = m[0].split('').map.with_index   { |x, i| c_label(i) }
  end

  def swap_columns(i, j)
    @c_label[i], @c_label[j] = @c_label[j], @c_label[i]
    @matrix[i], @matrix[j] = @matrix[j], @matrix[i]
  end

  # Sort columns in decreasing lexicographic order
  def sort_columns
    @c_labels = Array.new(@matrix.keys)
    @c_labels.sort! { |a,b| @matrix[b] <=> @matrix[a] }
  end

  def remove_null
    @matrix.each_pair do |c, chars|
      if chars =~ /^0+$/
        @matrix.delete(c)
        @c_labels.delete(c)
      end
      @c_num = @matrix.keys.length
    end
  end

  def species(character)
    # Output the list of species that have the input character
    column = @matrix[character]
    return column.length.times.select { |p| column[p] == '1'}
  end

  def characters(species)
    # Output the list of characters that the input species possesses
    return @matrix.keys.select { |k| @matrix[k][species] == '1' }
  end

  # Considering only the columns corresponding to the input characters,
  # find the list of maximal characters
  def find_maximal_chars(all_characters)
    maximals = Array.new()
    while not all_characters.empty?
      maximals.push(all_characters.first)
      species_first_char = species(all_characters.first)
      block_chars = species_first_char.map { |s| characters(s) }
      union = block_chars.inject([]) { |old, new| old.concat(new) }.uniq
      union.map { |c|  all_characters.delete(c) }
    end
    return maximals
  end

  # Partition the characters in the portions starting with the maximal characters
  # i.e. each maximal characters begins a new portion
  def partition(characters_set)
    # Copy the characters_set array, otherwise it is passed as reference
    maximals = find_maximal_chars(Marshal.load(Marshal.dump(characters_set)))
    classes = Array.new
    # For each maximal character m, get the species that have m
    # If the matrix has a perfect phylogeny, the union of the characters possessed
    # by those species is a class of the partition
    maximals.map { |m| species(m) }.each do |species_set|
      char_set = species_set.map.inject(Set.new) { |s, x| s.union(characters(x).to_set) }.intersection(characters_set.to_set)
      char_array = char_set.to_a
      # puts "characters_set: #{characters_set}"
      # puts "maximals: #{maximals}"
      # puts "char_array: #{char_array}"
      classes.push(char_array.sort { |a,b| characters_set.index(a) <=> characters_set.index(b) } )
    end
    return classes
  end
end

def build_tree(matrix, characters)
  # tree is the tree currently built, the subtree computed will be grafted at root
  # characters is the sorted list of characters to be realized
  # We will compute the subtree on the input characters
  if characters.size == 0
    # If characters.size == 0, then there is no subtree to compute
    return ''
  end
  subtrees = matrix.partition(characters)
  inner = subtrees.map do |characters_subtree|
    # Since partition splits the characters into disconnected parts of the
    # input matrix, each class of the partition corresponds to a separate subtree.
    # We realize the first character of each subtree, that is the maximal character
    # of each component.
    # Then we recurse on each component.
    c = characters_subtree.shift
    rest = build_tree(matrix, characters_subtree)
    "#{rest}\:#{c}"
  end.join(',')
  return "\(#{inner}\)"
end



m = LabeledMatrix.new(File.readlines(options.matrix), options.persistent)
m.remove_null
m.sort_columns
#m.pp
puts "#{build_tree(m, m.c_labels)};"

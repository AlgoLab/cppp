#!/usr/bin/env ruby

require 'fileutils'

[10, 20, 40, 60].each do |n|
  [2*n/4, 3*n/4, 4*n/4, 5*n/4, 6*n/4].each do |m|
    dirname = "exp/#{n}/#{m}"
    system("mkdir -p #{dirname}") unless Dir.exists?(dirname)
    100.times do |i|
      unless File.exists?("#{dirname}/#{i}.ms")
        system "touch #{dirname}/#{i}.ms"
        system "./ms #{n} 1 -s #{m} -r #{4 * 0.025 * n * m} #{m}  | tail -n +7 | sort | uniq -c > #{dirname}/#{i}.ms"
        system "./ms2ppp #{dirname}/#{i}.ms > #{dirname}/#{i}.data"
        system "/usr/bin/time -f \"%e\" -o #{dirname}/#{i}.log timeout 10m ./cppp  #{dirname}/#{i}.data > #{dirname}/#{i}.out"
      end
      [1, 2, 4, 8, 16].each do |r|
        mod_dirname = "mod/#{n}/#{m}/#{i}/#{r}"
        system("mkdir -p #{mod_dirname}") unless Dir.exists?(mod_dirname)
        10.times do |x|
          unless File.exists?("#{mod_dirname}/#{x}.data")
            system "touch #{mod_dirname}/#{x}.data"
            system "./random-ppp.rb -r #{r} -m #{dirname}/#{i}.data > #{mod_dirname}/#{x}.data"
            system "/usr/bin/time -f \"%e\" -o #{mod_dirname}/#{x}.log timeout 10m ./cppp  #{mod_dirname}/#{x}.data > #{mod_dirname}/#{x}.out"
          end
        end
      end
    end
  end
end

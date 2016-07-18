# = Rakefile for como library.

require 'rake/testtask'

Rake::TestTask.new do |t|
    t.libs << 'test'
end

task :clean_test do
    sh "rm -f test/result/*"
    sh "rm -f test/como_options"
    sh "rm -f test/como_config"
    sh "rm -f test/como_subcmd"
    sh "rm -f test/como_type_prim"
end

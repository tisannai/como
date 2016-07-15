require 'test/unit'
require 'stringio'

# Test execution routine.
def runTest( test )
    Dir.chdir( 'test' )
    FileUtils.mkdir_p "result"
    rf = "result/#{test}.txt"
    gf = "golden/#{test}.txt"

    system( "gcc -Wall -std=gnu99 -g como_#{test}.c ../src/.libs/libcomo.a -o como_#{test}" )

    system( "rm -f #{rf}; touch #{rf}" )

    File.open( "test_#{test}" ).readlines.each do |cmd|
        cmd = cmd.chomp
        next if cmd.empty?
        system( "echo \"---- CMD: #{cmd}\" >> #{rf}" )
        system( "export RUBYLIB=../lib; #{cmd} >> #{rf} 2>&1" )
    end

    assert( system( "diff #{rf} #{gf}" ), "FAILED: diff #{rf} #{gf}" )

    Dir.chdir( '..' )
end


class ComoTest < Test::Unit::TestCase

    def test_options()       runTest( "options"       ); end
    def test_config()        runTest( "config"        ); end
    def test_subcmd()        runTest( "subcmd"        ); end
    def test_type_prim()        runTest( "type_prim"        ); end

end

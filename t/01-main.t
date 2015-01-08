#!/usr/bin/perl
use Dir::Self;
use lib __DIR__ . "../lib";
use lib __DIR__ . "../";
BEGIN {
    eval {
        require Carp::Always;
        Carp::Always->import();
    };
}

$Log::Fu::LINE_PREFIX = '#';

my $jarurl = 'http://files.couchbase.com/maven2/org/couchbase/mock/CouchbaseMock/0.8-SNAPSHOT/CouchbaseMock-0.8-20140621.030439-1.jar';
my $jarfile = __DIR__ . "/tmp/CouchbaseMock.jar";

if (! -e $jarfile) {
    warn("Can't find JAR. Downloading.. $jarurl");
    system("wget -O $jarfile $jarurl");
}

use Couchbase::Test::Common;
Couchbase::Test::Common->Initialize(
    jarfile => $jarfile,
    nodes => 5,
    buckets => [{name => "default", type => "couchbase"}],
);

use Couchbase::Test::ClientSync;
use Couchbase::Test::Settings;
use Couchbase::Test::Views;

Couchbase::Test::ClientSync->runtests();
Couchbase::Test::Settings->runtests();
Couchbase::Test::Views->runtests();
#Test::Class->runtests();

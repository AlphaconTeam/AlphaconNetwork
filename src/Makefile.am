# Copyright (c) 2013-2016 The Bitcoin Core developers
# Copyright (c) 2017 The Alphacon Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

DIST_SUBDIRS = secp256k1 univalue

AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS)
AM_CXXFLAGS = $(HARDENED_CXXFLAGS) $(ERROR_CXXFLAGS)
AM_CPPFLAGS = $(HARDENED_CPPFLAGS)
EXTRA_LIBRARIES =

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

ALPHACON_INCLUDES=-I$(builddir) -I$(builddir)/obj $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS)

ALPHACON_INCLUDES += -I$(srcdir)/secp256k1/include
ALPHACON_INCLUDES += $(UNIVALUE_CFLAGS)

LIBALPHACON_SERVER=libalphacon_server.a
LIBALPHACON_COMMON=libalphacon_common.a
LIBALPHACON_CONSENSUS=libalphacon_consensus.a
LIBALPHACON_CLI=libalphacon_cli.a
LIBALPHACON_UTIL=libalphacon_util.a
LIBALPHACON_CRYPTO=crypto/libalphacon_crypto.a
LIBALPHACONQT=qt/libalphaconqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la

if ENABLE_ZMQ
LIBALPHACON_ZMQ=libalphacon_zmq.a
endif
if BUILD_ALPHACON_LIBS
LIBALPHACONCONSENSUS=libalphaconconsensus.la
endif
if ENABLE_WALLET
LIBALPHACON_WALLET=libalphacon_wallet.a
endif

$(LIBSECP256K1): $(wildcard secp256k1/src/*) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBALPHACON_CRYPTO) \
  $(LIBALPHACON_UTIL) \
  $(LIBALPHACON_COMMON) \
  $(LIBALPHACON_CONSENSUS) \
  $(LIBALPHACON_SERVER) \
  $(LIBALPHACON_CLI) \
  $(LIBALPHACON_WALLET) \
  $(LIBALPHACON_ZMQ)

lib_LTLIBRARIES = $(LIBALPHACONCONSENSUS)

bin_PROGRAMS =
noinst_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_ALPHACOND
  bin_PROGRAMS += alphacond
endif

if BUILD_ALPHACON_CLI
  bin_PROGRAMS += alphacon-cli
endif

if BUILD_ALPHACON_TX
  bin_PROGRAMS += alphacon-tx
endif

.PHONY: FORCE check-symbols check-security
# alphacon core #
ALPHACON_CORE_H = \
  addrdb.h \
  addressindex.h \
  spentindex.h \
  timestampindex.h \
  addrman.h \
  assets/assets.h \
  assets/assetdb.h \
  assets/assettypes.h \
  base58.h \
  bloom.h \
  blockencodings.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coins.h \
  compat.h \
  compat/byteswap.h \
  compat/endian.h \
  compat/sanity.h \
  compressor.h \
  consensus/consensus.h \
  consensus/tx_verify.h \
  core_io.h \
  core_memusage.h \
  cuckoocache.h \
  fs.h \
  httprpc.h \
  httpserver.h \
  indirectmap.h \
  init.h \
  key.h \
  keystore.h \
  dbwrapper.h \
  limitedmap.h \
  memusage.h \
  merkleblock.h \
  miner.h \
  net.h \
  net_processing.h \
  netaddress.h \
  netbase.h \
  netmessagemaker.h \
  noui.h \
  policy/feerate.h \
  policy/fees.h \
  policy/policy.h \
  policy/rbf.h \
  pos.h \
  pow.h \
  protocol.h \
  random.h \
  reverse_iterator.h \
  reverselock.h \
  rpc/blockchain.h \
  rpc/client.h \
  rpc/mining.h \
  rpc/protocol.h \
  rpc/safemode.h \
  rpc/server.h \
  rpc/register.h \
  scheduler.h \
  script/sigcache.h \
  script/sign.h \
  script/standard.h \
  script/ismine.h \
  streams.h \
  support/allocators/secure.h \
  support/allocators/zeroafterfree.h \
  support/cleanse.h \
  support/events.h \
  support/lockedpool.h \
  sync.h \
  threadsafety.h \
  threadinterrupt.h \
  timedata.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  undo.h \
  util.h \
  utilmoneystr.h \
  utiltime.h \
  validation.h \
  validationinterface.h \
  versionbits.h \
  wallet/coincontrol.h \
  wallet/crypter.h \
  wallet/db.h \
  wallet/feebumper.h \
  wallet/fees.h \
  wallet/init.h \
  wallet/rpcwallet.h \
  wallet/wallet.h \
  wallet/walletdb.h \
  warnings.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqconfig.h\
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h


obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh "$(abs_top_builddir)/src/obj/build.h" \
	  "$(abs_top_srcdir)"
libalphacon_util_a-clientversion.$(OBJEXT): obj/build.h

# server: shared between alphacond and alphacon-qt
libalphacon_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(ALPHACON_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libalphacon_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libalphacon_server_a_SOURCES = \
  addrdb.cpp \
  addrman.cpp \
  bloom.cpp \
  blockencodings.cpp \
  chain.cpp \
  checkpoints.cpp \
  consensus/consensus.cpp \
  consensus/tx_verify.cpp \
  httprpc.cpp \
  httpserver.cpp \
  init.cpp \
  dbwrapper.cpp \
  merkleblock.cpp \
  miner.cpp \
  net.cpp \
  net_processing.cpp \
  noui.cpp \
  assets/assets.cpp \
  assets/assetdb.cpp \
  assets/assettypes.cpp \
  policy/fees.cpp \
  policy/policy.cpp \
  policy/rbf.cpp \
  pos.cpp \
  pow.cpp \
  rest.cpp \
  rpc/assets.cpp \
  rpc/blockchain.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/safemode.cpp \
  rpc/server.cpp \
  script/sigcache.cpp \
  script/ismine.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txmempool.cpp \
  ui_interface.cpp \
  validation.cpp \
  validationinterface.cpp \
  versionbits.cpp \
  $(ALPHACON_CORE_H)

if ENABLE_ZMQ
libalphacon_zmq_a_CPPFLAGS = $(ALPHACON_INCLUDES) $(ZMQ_CFLAGS)
libalphacon_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libalphacon_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp
endif


# wallet: shared between alphacond and alphacon-qt, but only linked
# when wallet enabled
libalphacon_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(ALPHACON_INCLUDES)
libalphacon_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libalphacon_wallet_a_SOURCES = \
  wallet/crypter.cpp \
  wallet/db.cpp \
  wallet/feebumper.cpp \
  wallet/fees.cpp \
  wallet/init.cpp \
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  wallet/wallet.cpp \
  wallet/walletdb.cpp \
  $(ALPHACON_CORE_H)

# crypto primitives library
crypto_libalphacon_crypto_a_CPPFLAGS = $(AM_CPPFLAGS)
crypto_libalphacon_crypto_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
crypto_libalphacon_crypto_a_SOURCES = \
  crypto/aes.cpp \
  crypto/aes.h \
  crypto/chacha20.h \
  crypto/chacha20.cpp \
  crypto/common.h \
  crypto/hmac_sha256.cpp \
  crypto/hmac_sha256.h \
  crypto/hmac_sha512.cpp \
  crypto/hmac_sha512.h \
  crypto/ripemd160.cpp \
  crypto/aes_helper.c \
  crypto/groestl.c \
  crypto/sph_sha2.c \
  crypto/sph_sha512.c \
  crypto/ripemd160.h \
  crypto/sha1.cpp \
  crypto/sha1.h \
  crypto/sha256.cpp \
  crypto/sha256.h \
  crypto/sha512.h \
  crypto/sph_groestl.h \
  crypto/sph_sha2.h \
  crypto/sph_types.h \
  crypto/sha512.cpp \
  crypto/sha512.h

if USE_ASM
crypto_libalphacon_crypto_a_SOURCES += crypto/sha256_sse4.cpp
endif

# consensus: shared between all executables that validate any consensus rules.
libalphacon_consensus_a_CPPFLAGS = $(AM_CPPFLAGS) $(ALPHACON_INCLUDES)
libalphacon_consensus_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libalphacon_consensus_a_SOURCES = \
  amount.h \
  arith_uint256.cpp \
  arith_uint256.h \
  consensus/merkle.cpp \
  consensus/merkle.h \
  consensus/params.h \
  consensus/validation.h \
  hash.cpp \
  hash.h \
  prevector.h \
  primitives/block.cpp \
  primitives/block.h \
  primitives/transaction.cpp \
  primitives/transaction.h \
  pubkey.cpp \
  pubkey.h \
  script/alphaconconsensus.cpp \
  script/interpreter.cpp \
  script/interpreter.h \
  script/script.cpp \
  script/script.h \
  script/script_error.cpp \
  script/script_error.h \
  serialize.h \
  support/cleanse.h \
  support/cleanse.cpp \
  tinyformat.h \
  uint256.cpp \
  uint256.h \
  utilstrencodings.cpp \
  utilstrencodings.h \
  version.h

# common: shared between alphacond, and alphacon-qt and non-server tools
libalphacon_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(ALPHACON_INCLUDES)
libalphacon_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libalphacon_common_a_SOURCES = \
  base58.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  core_read.cpp \
  core_write.cpp \
  key.cpp \
  keystore.cpp \
  netaddress.cpp \
  netbase.cpp \
  policy/feerate.cpp \
  protocol.cpp \
  scheduler.cpp \
  script/sign.cpp \
  script/standard.cpp \
  warnings.cpp \
  $(ALPHACON_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libalphacon_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(ALPHACON_INCLUDES)
libalphacon_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libalphacon_util_a_SOURCES = \
  support/lockedpool.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  compat/strnlen.cpp \
  fs.cpp \
  random.cpp \
  rpc/protocol.cpp \
  support/cleanse.cpp \
  sync.cpp \
  threadinterrupt.cpp \
  util.cpp \
  utilmoneystr.cpp \
  utilstrencodings.cpp \
  utiltime.cpp \
  $(ALPHACON_CORE_H)

if GLIBC_BACK_COMPAT
libalphacon_util_a_SOURCES += compat/glibc_compat.cpp
AM_LDFLAGS += $(COMPAT_LDFLAGS)
endif

# cli: shared between alphacon-cli and alphacon-qt
libalphacon_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(ALPHACON_INCLUDES)
libalphacon_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libalphacon_cli_a_SOURCES = \
  rpc/client.cpp \
  $(ALPHACON_CORE_H)

nodist_libalphacon_util_a_SOURCES = $(srcdir)/obj/build.h
#

# alphacond binary #
alphacond_SOURCES = alphacond.cpp
alphacond_CPPFLAGS = $(AM_CPPFLAGS) $(ALPHACON_INCLUDES)
alphacond_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
alphacond_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
alphacond_SOURCES += alphacond-res.rc
endif

alphacond_LDADD = \
  $(LIBALPHACON_SERVER) \
  $(LIBALPHACON_COMMON) \
  $(LIBUNIVALUE) \
  $(LIBALPHACON_UTIL) \
  $(LIBALPHACON_WALLET) \
  $(LIBALPHACON_ZMQ) \
  $(LIBALPHACON_CONSENSUS) \
  $(LIBALPHACON_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBSECP256K1)

alphacond_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS)

# alphacon-cli binary #
alphacon_cli_SOURCES = alphacon-cli.cpp
alphacon_cli_CPPFLAGS = $(AM_CPPFLAGS) $(ALPHACON_INCLUDES) $(EVENT_CFLAGS)
alphacon_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
alphacon_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
alphacon_cli_SOURCES += alphacon-cli-res.rc
endif

alphacon_cli_LDADD = \
  $(LIBALPHACON_CLI) \
  $(LIBUNIVALUE) \
  $(LIBALPHACON_UTIL) \
  $(LIBALPHACON_CRYPTO)

alphacon_cli_LDADD += $(BOOST_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(EVENT_LIBS)
#

# alphacon-tx binary #
alphacon_tx_SOURCES = alphacon-tx.cpp
alphacon_tx_CPPFLAGS = $(AM_CPPFLAGS) $(ALPHACON_INCLUDES)
alphacon_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
alphacon_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
alphacon_tx_SOURCES += alphacon-tx-res.rc
endif

alphacon_tx_LDADD = \
  $(LIBUNIVALUE) \
  $(LIBALPHACON_COMMON) \
  $(LIBALPHACON_UTIL) \
  $(LIBALPHACON_CONSENSUS) \
  $(LIBALPHACON_CRYPTO) \
  $(LIBSECP256K1)

alphacon_tx_LDADD += $(BOOST_LIBS) $(CRYPTO_LIBS)
#

# alphaconconsensus library #
if BUILD_ALPHACON_LIBS
include_HEADERS = script/alphaconconsensus.h
libalphaconconsensus_la_SOURCES = $(crypto_libalphacon_crypto_a_SOURCES) $(libalphacon_consensus_a_SOURCES)

if GLIBC_BACK_COMPAT
  libalphaconconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

libalphaconconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
libalphaconconsensus_la_LIBADD = $(LIBSECP256K1)
libalphaconconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_ALPHACON_INTERNAL
libalphaconconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

endif
#

CTAES_DIST =  crypto/ctaes/bench.c
CTAES_DIST += crypto/ctaes/ctaes.c
CTAES_DIST += crypto/ctaes/ctaes.h
CTAES_DIST += crypto/ctaes/README.md
CTAES_DIST += crypto/ctaes/test.c

CLEANFILES = $(EXTRA_LIBRARIES)

CLEANFILES += *.gcda *.gcno
CLEANFILES += compat/*.gcda compat/*.gcno
CLEANFILES += consensus/*.gcda consensus/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += policy/*.gcda policy/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno
CLEANFILES += obj/build.h

EXTRA_DIST = $(CTAES_DIST)


config/alphacon-config.h: config/stamp-h1
	@$(MAKE) -C $(top_builddir) $(subdir)/$(@)
config/stamp-h1: $(top_srcdir)/$(subdir)/config/alphacon-config.h.in $(top_builddir)/config.status
	$(AM_V_at)$(MAKE) -C $(top_builddir) $(subdir)/$(@)
$(top_srcdir)/$(subdir)/config/alphacon-config.h.in:  $(am__configure_deps)
	$(AM_V_at)$(MAKE) -C $(top_srcdir) $(subdir)/config/alphacon-config.h.in

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C univalue clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h
	-rm -rf test/__pycache__

.rc.o:
	@test -f $(WINDRES)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

check-symbols: $(bin_PROGRAMS)
if GLIBC_BACK_COMPAT
	@echo "Checking glibc back compat..."
	$(AM_V_at) READELF=$(READELF) CPPFILT=$(CPPFILT) $(top_srcdir)/contrib/devtools/symbol-check.py < $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) READELF=$(READELF) OBJDUMP=$(OBJDUMP) $(top_srcdir)/contrib/devtools/security-check.py < $(bin_PROGRAMS)
endif

%.pb.cc %.pb.h: %.proto
	@test -f $(PROTOC)
	$(AM_V_GEN) $(PROTOC) --cpp_out=$(@D) --proto_path=$(<D) $<

if EMBEDDED_LEVELDB
include Makefile.leveldb.include
endif

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_BENCH
include Makefile.bench.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif

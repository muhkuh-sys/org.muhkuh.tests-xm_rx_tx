----------------------------------------------------------------------------
--
-- BAM Manual : https://matricks.github.io/bam/bam.html
--

-- Provide Penlight.
local pl = require'pl.import_into'()


-----------------------------------------------------------------------------
--
-- Setup the Muhkuh build system.
--
local atEnv = require "mbs"

----------------------------------------------------------------------------------------------------------------------
--
-- Create all environments.
--

-- Set some variables.
-- FIXME: Move this to build.properties and CLI.
atEnv.DEFAULT.atVars.BUILD_TYPE = 'RELEASE'


-- FIXME: Move this to setup.json file
atEnv.DEFAULT.atVars.PROJECT_VERSION =
{
  "1", "0", "0"
}

-- FIXME: Move this to setup.json file
--  tools

atEnv.NETX500 = atEnv.DEFAULT:CreateEnvironment{'gcc-arm-none-eabi-4.7'}
  :AddCompiler('NETX500')

-- atEnv.NETX50 = atEnv.DEFAULT:CreateEnvironment{'gcc-arm-none-eabi-4.7'}
--   :AddCompiler('NETX50')

-- atEnv.NETX56 = atEnv.DEFAULT:CreateEnvironment{'gcc-arm-none-eabi-4.7'}
--   :AddCompiler('NETX56')


----------------------------------------------------------------------------------------------------------------------
--
-- Build the platform library.
--

SubBAM('platform/bam.lua',atEnv)


--------------------------------------------------------------------------------------------------------------
--
-- Filter the version file.
--

local tVersionFile = atEnv.DEFAULT:VersionTemplate('targets/version/version.h', 'templates/version.h')


--------------------------------------------------------------------------------------------------------------
--
-- This is the list of sources.
--

local tSources_common = pl.List({
  'src/header.c',
  'src/init_muhkuh.S',
  'src/main_test.c',
})

local tSources_netx500 = pl.List({
  'src/netx500/xc_reset.c',
  'src/netx500/xm_rx_tx.c'
})


--------------------------------------------------------------------------------------------------------------
--
-- Build crypto.
--

local astrIncludePaths =
{
  'src',
  'platform/src',
  'platform/src/lib',
  'targets/version'
}

local atBuildConfigurations = {
  netX500 =
  {
    BASE_ENV = atEnv.NETX500,
    LDFILE   = 'src/netx500/netx500.ld',
    SOURCE   = tSources_common .. tSources_netx500,
    BIN_NAME = string.format('xm_rx_tx_%s', string.lower(atEnv.NETX500.atVars.COMPILER_ID)),
  }
}

-- Collect the build results in the environment.
local tBuildEnv = {}

-- Create ELF file, ObjCopy file and ObjDump file for the netX90 communication CPU.
for strBuildName, atBuildAttributes in pairs(atBuildConfigurations) do
  -- Create a new environment based on BASE_ENV
  local tEnv = atBuildAttributes.BASE_ENV:Clone()
  tBuildEnv[strBuildName] = tEnv

  -- Set include paths for the platform lib.
  tEnv:AddInclude(astrIncludePaths)

  -- Set ouput path for all sources in "src".
  tEnv:SetBuildPath(
    'src',
    pl.path.join("targets",strBuildName,'build')
  )

  -- Build all sources.
  local atObjects = tEnv:Compile(atBuildAttributes.SOURCE)

  -- Add the extension of ELF file
  tEnv:SetLinkExtension(".elf")

  -- Now link the libraries to an ELF file.
  local tElf = tEnv:Link(
    pl.path.join("targets",strBuildName,strBuildName .. ".elf"),
    atBuildAttributes.LDFILE,
    atObjects,
    tEnv.atVars.PLATFORM_LIB
  )

  -- Create a complete dump of the ELF file.
  local tTxt = tEnv:ObjDump(
    pl.path.join("targets",strBuildName,strBuildName .. ".txt"),
    tElf
  )

  -- Create a binary from the ELF file.
  local tBin = tEnv:ObjCopy(
    pl.path.join("targets",strBuildName,atBuildAttributes.BIN_NAME .. ".bin"),
    tElf
  )

  -- Store the build in the environment.
  tEnv.atVars.BIN_PATH = tBin
end

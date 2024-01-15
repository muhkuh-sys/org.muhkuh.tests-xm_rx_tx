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
  "2", "0", "0"
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

-- Create ELF file, ObjCopy file and ObjDump file for all targets.
for strBuildName, atBuildAttributes in pairs(atBuildConfigurations) do
  local strBuildNameLower = string.lower(strBuildName)

  -- Create a new environment based on BASE_ENV
  local tEnv = atBuildAttributes.BASE_ENV:Clone()
  tBuildEnv[strBuildName] = tEnv

  -- Set include paths for the platform lib.
  tEnv:AddInclude(astrIncludePaths)

  -- Set ouput path for all sources in "src".
  tEnv:SetBuildPath(
    'src',
    pl.path.join("targets", strBuildNameLower, 'build')
  )

  -- Build all sources.
  local atObjects = tEnv:Compile(atBuildAttributes.SOURCE)

  -- Add the extension of ELF file
  tEnv:SetLinkExtension(".elf")

  -- Now link the libraries to an ELF file.
  local tElf = tEnv:Link(
    pl.path.join("targets", strBuildNameLower, strBuildNameLower .. ".elf"),
    atBuildAttributes.LDFILE,
    atObjects,
    tEnv.atVars.PLATFORM_LIB
  )

  -- Create a complete dump of the ELF file.
  local tTxt = tEnv:ObjDump(
    pl.path.join("targets", strBuildNameLower, strBuildNameLower .. ".txt"),
    tElf
  )

  -- Create a binary from the ELF file.
  local tBin = tEnv:ObjCopy(
    pl.path.join("targets", strBuildNameLower, atBuildAttributes.BIN_NAME .. ".bin"),
    tElf
  )

  -- Store the build in the environment.
  tEnv.atVars.BIN_PATH = tBin
end

------------------------------------------------------------------------------
--
-- Build the artifacts of crypto_base, test_crypto_personalisation,
--

-- FIXME: Get all information except the structure from the artifact configuration.
-- FIXME: The MBS2 system should read the group and module from a setup file.
-- FIXME: An environment should have something like "PROJECT_VERSION_PRETTY"
--        with a string representation of the project version.
local atArtifacts =
{
  strRepositoryPath = 'targets/jonchki/repository',

  {
    strGroup = 'org.muhkuh.tests',
    strModule = 'xm_rx_tx',
    strArtifact = 'xm_rx_tx',
    strProject_version = table.concat(atEnv.DEFAULT.atVars.PROJECT_VERSION,'.'),
    archive = {
      structure = {
        ['netx'] = {
          'targets/netx500/xm_rx_tx_netx500.bin'
        },
        'installer/org.muhkuh.tests-xm_rx_tx/install.lua'
      },
      extensions = {'tar', 'xz'},
      format = 'tar',
      filter = {'xz'}
    },
    templates = {
      artifact_configuration = 'installer/org.muhkuh.tests-xm_rx_tx/xm_rx_tx.xml',
      pom = 'installer/org.muhkuh.tests-xm_rx_tx/pom.xml'
    },
    tHash_ID = {'md5','sha1','sha224','sha256','sha384','sha512'}
  }
}


-----------------------------------------------------------------------------
-- FIXME: Create a builder from the code below.

-- FIXME: Get all information from the artifact configuration.
-- FIXME: Changing the XML template does not trigger a rebuild of the output file.
-- FIXME: The POM file does not need a template file. It can be generated completely in the builder.
-- FIXME: Changing the POM template does not trigger a rebuild of the output file.
--        This is not important as the POM should be generated without a template anyway.

local strHash_template = '${ID_UC}:${HASH}\n' -- the builder hash use given Replacements!
local strRepositoryPath = atArtifacts.strRepositoryPath

local atGeneratedFiles = {}

local path = require 'pl.path'
local stringx = require 'pl.stringx'
for _, tArtifactCfg in ipairs(atArtifacts) do
  -- Get the artifact ID and version.
  local strArtifact = tArtifactCfg.strArtifact
  local strProjectVersion = tArtifactCfg.strProject_version
  -- Get the list of hash IDs to generate.
  local tHashIDs = tArtifactCfg.tHash_ID

  -- Get the group as a path.
  local strGroupPath = path.join(
    table.unpack(
      stringx.split(tArtifactCfg.strGroup,'.')
    )
  )
  -- Build the output path for all files.
  local strArtifactPath = path.join(
    strRepositoryPath,
    strGroupPath,
    tArtifactCfg.strModule,
    tArtifactCfg.strProject_version
  )

  -- Get a shortcut to the archive settings.
  local tArchiveCfg = tArtifactCfg.archive

  -- Build the archive
  local strArchiveOutputPath = path.join(
    strArtifactPath,
    string.format(
      '%s-%s',
      strArtifact,
      strProjectVersion
    ) .. '.' .. table.concat(tArchiveCfg.extensions,'.')
  )
  local tArtifact = atEnv.DEFAULT:Archive(
    strArchiveOutputPath,
    tArchiveCfg.format,
    tArchiveCfg.filter,
    tArchiveCfg.structure
  )
  table.insert(atGeneratedFiles, tArtifact)

  -- Build hash of archive
  local tArtifactHash = atEnv.DEFAULT:Hash(
    string.format('%s.hash', tArtifact),
    tArtifact,
    tHashIDs,
    strHash_template
  )
  table.insert(atGeneratedFiles, tArtifactHash)

  local strConfigurationOutputPath = path.join(
    strArtifactPath,
    string.format(
      '%s-%s.xml',
      strArtifact,
      strProjectVersion
    )
  )
  local tConfiguration = atEnv.DEFAULT:VersionTemplate(
    strConfigurationOutputPath,
    tArtifactCfg.templates.artifact_configuration
  )
  table.insert(atGeneratedFiles, tConfiguration)

  local tConfigurationHash = atEnv.DEFAULT:Hash(
    string.format('%s.hash', tConfiguration),
    tConfiguration,
    tHashIDs,
    strHash_template
  )
  table.insert(atGeneratedFiles, tConfigurationHash)

  local strPomOutputPath = path.join(
    strArtifactPath,
    string.format(
      '%s-%s.pom',
      strArtifact,
      strProjectVersion
    )
  )
  local tArtifactPom = atEnv.DEFAULT:VersionTemplate(
    strPomOutputPath,
    tArtifactCfg.templates.pom
  )
  table.insert(atGeneratedFiles, tArtifactPom)
end

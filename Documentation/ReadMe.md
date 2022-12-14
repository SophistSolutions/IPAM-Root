# Design Overview for IPAM

## Inspiration/Use case

Inspiring idea/use case, we are first exploring.

We want to be able to store all Sterlings (and friends/family's) photos in a fairly persistent fasion,
on the web. And we don't want someone elses rules (like Facebook) to control who has access to what.

We want to integrate with existing photo album tools, be able to skim data from them, index/tag based
on the organizaiton that already exists, and persist that in a way that we can share bundles of photos
with whomever we choose.

IPAM itself will not alter the underlying document in any fashion. Changes to the metadata will be stored
separately from the document itself (a requirement for most non-image files, and a choice for image files).

## Inspiration/Use case II

Managing health/medical records is an analagous application (see HealthFrame). In that realm, you have a variety of marginally compatible systems and langauges. It is helpful to preserve all original documents, and reference them. It is helpful to be able to share
summaries that are locked/secured differently. Much of the IPFS design (see below) can be the done in similar fashion.

## Technology

Leverage IPFS. Use this to publish/store any content. Leverage its ability to create names (IPNS) to
handle the publishing/sharing.
** something about rollups (ala blockchain) here or below?? **

### ** Terminology **

- File (here FILE refers to what the person originally adds to the system, NOT an IPAM document
- document
  - the unit IPAM considers the basic item being indexed. This is versioned, has many 'sections'
    and sub-parts, but its the level at which the indexer has decided (undecided ) FILE which represnets teh tneitre docuemnt)
- section
  A section is a portion of a document.

- metadata -- defined xxx, associated with individual documents
- actor
  - sub-types
    - person
    - bot/system
  - instances
    - owner (can make changes)
    - viewer
    - indexer (automated bot)
- subject
  - a thing the document is about
  - person, concept, pet, location, ...
- thumbnail
  - a section providing a small summary (sometimes automatically derived) data about a document
  - e.g. small image

### Document model

- Aliases for Document: File

This is where the secret sauce lies (and abstraction here is why this will also work for medical records, or
nearly any other kind of data).

Define abstract schemas with mechanisms for tagging, FILEs (assocating extensible named attribtutes).

Some permissions mechanism (I forget the details - to the extent to which I had worked them out).

** Document details **
physical file on a file system
associated metadata
'creator' assignable permissions
user assignable 'thumbnail'

#### VERSIONING

Each version of a file has a CID. Each GENERIC-FILE has a NAME (strong name), whcih conists of a CURRENT VERSION
and BASED-ON version.

This way, you can always trace back and undo

#### SECTIONS

Each section of a file can optionally be ENCRYPTED. This way - ALL data is PUBLICLY available. What you use
to control access is management of KEYS (NOT WORKED OUT HOW TO HANDLE THIS).

It would be NICE if there was a way for keys to expire (technically not fully possible, but maybe pragmatically possible - like
with DRM).

#### API-Server

We will construct an API which allows easy access to POSTING, and UPDATING FILE objects (essentially the album manager).
It will have ZERO data storage for system of record. But will store database INDEXES for likely/interesting keys.

In this way, it can always RECONSTRUCT and load balance requests.

The API-Server is what talks to IPFS nodes.

#### AGENTS

You will install on your iphone or PC an agent, that watches directories, and PUSHES any file changes to the API-Server

In this way, the agents have a simple problem of finding data locally, and pushing/publishing it to the API-Server in the cloud.

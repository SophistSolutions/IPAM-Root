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

- File
  here FILE refers to what the person originally adds to the system, NOT an IPAM document
- Document
  - the unit IPAM considers the basic item being indexed. This is versioned, has many 'sections'
    and sub-parts, but its the level at which the indexer has decided (undecided) FILE will be grouped/considered.
  A document is a conceptual thing - not an actual file anyplace. The 'document' consists of all past and future
  versions of the data (maybe?).
- Versioned-Document-Instance
  The document conceptually has many versions. But at any point in time, you may examine a single version
  of the document. In fact, thats really mostly what the system operates on - is versioned document instances.
- Section
  A section is a portion of a Versioned-Document-Instance. Examples of sections include
  - Raw File
  - Metadata
    E.G.
    - Thumbnails (@todo could lump as part of metadata)
    - Version information
    - Tags

  Each section maybe separately permissioned, and encoded.
- Metadata
  - defined xxx, associated with individual documents
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
- tag
  - something that can be marked as relevant to a document, and they have an associated thumbmail
  - a (schema-)typed string about a document
    EXPLAINER
    A typical 'tag' you might person in an image.
    Tags have two parts: KEY and VALUE. The KEY in this case would be "Person" - but not the STRING - person. Rather, a
    predefined open enumeration of values. So we predefine things like Person, and what they mean. But thie list of things
    that can be the KEY for the tag is open. ANYONE can add ANY to that list. This is like with Microsoft COM, and the IID.

    THe interpretation of the VALUE, depends on the KEY. This is why its critical there be universal agreement about 
    some set of predefined KEYS (like Person).
- set of metadata properties (LGP calls them tag-like things) that Sterling believes should not be lumped together with tags
  - e.g. Date, Rating, Comments, Title, FileName, Albums
  - **aside - this represents an area of disagreement, LGP leans towards merging these concepts as kinds of tags, and
    am open to any other name besides tag, for this generalized thing**
  - Metadata is the best name for this.
    ** I THINK WE ARE CLOSE TO A RESOLUTION HERE - MERGE TAG and these other metadata propties and just call them metadata properties - not tags **
    ** The objection sterling has to this is that ALL tags need a 'thumbnail' and NONE of the otehr properties do; and the set of enumerated known meaningful metadata keys
       are well formed for some of these and he wants them treated specially**
- person
  is a human (possibly lump in other animals here). Often they will have an email address
  and ability to login/authenticate with OAUTH2.

  - Each person, will have a current personal-thumbnail, which is an image which can be used in user interfaces
    to represent that person. It can be an avatar. (???)

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


## Operations/Use Case Scenarios

### Merging two Versioned-Document-Instance into a single Document

Imagine two different users enter Alabama.png and Alabama2.jpg into the system.
At some point, someone receives both through sharing. They notice they are the same and want
them treated as the same (they both show up in a gallery view next to one another and are indistinguishable).

How to handle this problem? One idea is DELETE one (but which?). And what about immutability didn't you understand?
Another approach is to create a MERGE record, saying this image SUBSUMES this other image. And indexes and futurue
queries can then tell that a merge has happened (and undo if they wish or accept if they wish). This can be done with 
BasedOn: having potentially MULTIPLE values (the versioning scheme).

### Splitting a Versioned-Document-Instance into a two (orphaning)

This can happen if Sterling has added tags to our mutual photo of Croatia, and insists  on dumb names. I dont
like his names. So I want to mark those as unintertesting for me, and going forward think of this as my own
image, not subject to future updates from Sterling.

Simply create a new ID, and create some kind of annotation in the document instance to make clear though it is
BASED on a common document, its not to be treated as such for updates/future merges of new information purposes. MAYBE
using a differnt name than BasedOn?


### Tie in to IPFS

Each document-instance will be stored as an IPFS document.
(so has a corresponding CID).

TBD - we will use IPNS to track some sort of notion of lastest version for each 'document'.

Each document containss a BASED_ON field whose value is in IPFS CID.

API-Server will maintain local cache/database of indexed info for documents (indexed metadata) for queries.

API-Server will monitor/poll changes in content from IPFS to keep its indexes up to date.

Most apps wil NOT communiate with IPFS directly, but with API-Server to get or upload data/images/galleries/queries etc.
The API-Server does NO image storage EXCEPT in its role as a CACHE (and maybe configuraiton/control information).

Each SECTION MAYBE also be implemented as an IPFS document, having its own CID. The reason for this is to make
ACTUAL document instances small, and able to retain/re-use large sections/chunks just by pointer.

Also - since sections can (and typically will be) encrypted, the API-Server can store in its cache database decryped 
copies of this section information (be it thumbnails, or indexed sets of tags, or whatever).
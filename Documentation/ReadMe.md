# Design Overview for IPAM

## Inspiration/Use case

Inspiring idea/use case, we are first exploring.

We want to be able to store all Sterlings (and friends/family's) photos in a fairly persistent fasion, 
on the web. And we don't want someone elses rules (like Facebook) to control who has access to what.

We want to integrate with existing photo album tools, be able to skim data from them, index/tag based
on the organizaiton that already exists, and persist that in a way that we can share bundles of photos
with whomever we choose.

## Inspiration/Use case II

Less well thought out, but HealthFrame (user health data). This is essentially the same problem I faced
with HealthFrame (or at least the solution I have in mind to the first use case applies pretty well here).

## Technology

Leverage IPFS. Use this to publish/store any content. Leverage its ability to create names (IPNS) to 
handle the publishing/sharing.

### Document model

This is where the secret sauce lies (and abstraction here is why this will also work for medical records, or
nearly any other kind of data).

First - we have the abstraction of a '(STERL - what name did I come up with)' For now - Call basic 
abstraction a FILE (aka UNIT of information collected by an album).

Define abstract schemas with mechanisms for tagging, FILEs (assocating extensible named attribtutes).

Some permissions mechanism (I forget the details - to the extent to which I had worked them out).

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


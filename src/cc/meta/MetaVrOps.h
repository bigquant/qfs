//---------------------------------------------------------- -*- Mode: C++ -*-
// $Id$
//
// Created 2016/05/11
// Author: Mike Ovsiannikov
//
// Copyright 2016 Quantcast Corp.
//
// This file is part of Kosmos File System (KFS).
//
// Licensed under the Apache License, Version 2.0
// (the "License"); you may not use this file except in compliance with
// the License. You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.
//
// Veiwstamped replications RPCs..
//
//
//----------------------------------------------------------------------------

#ifndef META_VROPS_H
#define META_VROPS_H

#include "common/kfstypes.h"

#include "MetaRequest.h"
#include "LogWriter.h"
#include "MetaVrSM.h"
#include "util.h"

#include <errno.h>

namespace KFS
{
class Properties;

const char* const kMetaVrViewSeqFieldNamePtr       = "VV";
const char* const kMetaVrEpochSeqFieldNamePtr      = "VE";
const char* const kMetaVrCommittedViewFieldNamePtr = "VCV";
const char* const kMetaVrCommittedFieldNamePtr     = "VC";
const char* const kMetaVrNextLogSeqFieldNamePtr    = "VN";
const char* const kMetaVrStateFieldNamePtr         = "VS";

class MetaVrRequest : public MetaRequest
{
public:
    typedef MetaVrSM::Config::NodeId NodeId;

    MetaVrRequest(
        MetaOp    inOpType,
        LogAction inLogAction,
        seq_t     inOpSequence = -1)
        : MetaRequest(inOpType, inLogAction, inOpSequence),
          mEpochSeq(-1),
          mViewSeq(-1),
          mCommittedViewSeq(-1),
          mCommittedSeq(-1),
          mNextLogSeq(-1),
          mNodeId(-1),
          mRetCurEpochSeq(-1),
          mRetCurViewSeq(-1),
          mRetCommittedViewSeq(-1),
          mRetCommittedSeq(-1),
          mRetNextLogSeq(-1),
          mRetCurState(-1),
          mVrSMPtr(0),
          mRefCount(0)
    {
        MetaVrRequest::Ref();
        shortRpcFormatFlag = false;
    }

    seq_t  mEpochSeq;
    seq_t  mViewSeq;
    seq_t  mCommittedViewSeq;
    seq_t  mCommittedSeq;
    seq_t  mNextLogSeq;
    NodeId mNodeId;
    seq_t  mRetCurEpochSeq;
    seq_t  mRetCurViewSeq;
    seq_t  mRetCommittedViewSeq;
    seq_t  mRetCommittedSeq;
    seq_t  mRetNextLogSeq;
    int    mRetCurState;

    bool Validate() const
    {
        return (0 <= mEpochSeq && 0 <= mViewSeq);
    }
    template<typename T>
    static T& ParserDef(
        T& inParser)
    {
        return LogAndIoDefs(MetaRequest::ParserDef(inParser));
    }
    template<typename T>
    static T& LogIoDef(
        T& inParser)
    {
        return LogAndIoDefs(MetaRequest::LogIoDef(inParser));
    }
    void Request(
        ReqOstream& inStream) const;
    virtual bool start()
    {
        if (0 == status && ! Validate()) {
            status = -EINVAL;
        }
        return (0 == status);
    }
    virtual void HandleResponse(
        seq_t             inSeq,
        const Properties& inProps,
        NodeId            inId) = 0;
    void Ref()
        { mRefCount++; }
    void Unref()
    {
        if (--mRefCount <= 0) {
            delete this;
        }
    }
    virtual void handle()
        { /* nothing */ }
    MetaVrSM* GetVrSMPtr() const
        { return mVrSMPtr; }
    void SetVrSMPtr(
        MetaVrSM* inPtr)
        { mVrSMPtr = inPtr; }
protected:
    MetaVrSM* mVrSMPtr;
    int       mRefCount;

    virtual ~MetaVrRequest()
    {
        if (0 != mRefCount) {
            panic("~MetaVrRequest: invalid ref count");
        }
    }
    bool ResponseHeader(
        ReqOstream& inOs);
    virtual void response(
        ReqOstream& inOs)
    {
        ResponseHeader(inOs);
        if (0 <= mRetCurViewSeq) {
            inOs << kMetaVrViewSeqFieldNamePtr <<
                ":" << mRetCurViewSeq << "\r\n";
        }
        if (0 <= mRetCurEpochSeq) {
            inOs << kMetaVrEpochSeqFieldNamePtr <<
                ":" << mRetCurEpochSeq << "\r\n";
        }
        if (0 <= mRetCurState) {
            inOs << kMetaVrStateFieldNamePtr <<
                ":" << mRetCurState << "\r\n";
        }
        if (0 <= mRetCommittedViewSeq) {
            inOs << kMetaVrCommittedViewFieldNamePtr <<
                ":" << mRetCommittedViewSeq << "\r\n";
        }
        if (0 <= mRetCommittedSeq) {
            inOs << kMetaVrCommittedFieldNamePtr <<
                ":" << mRetCommittedSeq << "\r\n";
        }
        if (0 <= mRetNextLogSeq) {
            inOs << kMetaVrNextLogSeqFieldNamePtr <<
                ":" << mRetNextLogSeq << "\r\n";
        }
        inOs << "\r\n";
    }
    template<typename T>
    void HandleReply(
        T&                inReq,
        seq_t             inSeq,
        const Properties& inProps,
        NodeId            inNodeId)
    {
        if (mVrSMPtr) {
            mVrSMPtr->HandleReply(inReq, inSeq, inProps, inNodeId);
        }
    }
    virtual void ReleaseSelf()
        { Unref(); }
    template<typename T>
    static T& LogAndIoDefs(
        T& inParser)
    {
        return inParser
        .Def("E",  &MetaVrRequest::mEpochSeq,         seq_t(-1))
        .Def("V",  &MetaVrRequest::mViewSeq,          seq_t(-1))
        .Def("CV", &MetaVrRequest::mCommittedViewSeq, seq_t(-1))
        .Def("C",  &MetaVrRequest::mCommittedSeq,     seq_t(-1))
        .Def("L",  &MetaVrRequest::mNextLogSeq,       seq_t(-1))
        .Def("N",  &MetaVrRequest::mNodeId,           NodeId(-1))
        ;
    }
private:
    MetaVrRequest(
        const MetaVrRequest& inRequest);
    MetaVrRequest& operator=(
        const MetaVrRequest& inRequest);
};

class MetaVrStartViewChange : public MetaVrRequest
{
public:
    MetaVrStartViewChange()
        : MetaVrRequest(META_VR_START_VIEW_CHANGE, kLogIfOk)
        {}
    virtual ostream& ShowSelf(
        ostream& inOs) const
    {
        return (inOs <<
            "vr-start-view-change" <<
            " node: "      << mNodeId <<
            " epoch: "     << mEpochSeq <<
            " view: "      << mViewSeq <<
            " committed: " << mCommittedSeq
        );
    }
    virtual void HandleResponse(
        seq_t             inSeq,
        const Properties& inProps,
        NodeId            inNodeId)
        { HandleReply(*this, inSeq, inProps, inNodeId); }
protected:
    virtual ~MetaVrStartViewChange()
        {}
};

class MetaVrDoViewChange : public MetaVrRequest
{
public:
    NodeId mPimaryNodeId;

    MetaVrDoViewChange()
        : MetaVrRequest(META_VR_DO_VIEW_CHANGE, kLogIfOk),
          mPimaryNodeId(-1)
        {}
    virtual ostream& ShowSelf(
        ostream& inOs) const
    {
        return (inOs <<
            "vr-do-view-change" <<
            " node: "      << mNodeId <<
            " epoch: "     << mEpochSeq <<
            " view: "      << mViewSeq <<
            " committed: " << mCommittedSeq <<
            " primary: "   << mPimaryNodeId
        );
    }
    virtual void HandleResponse(
        seq_t             inSeq,
        const Properties& inProps,
        NodeId            inNodeId)
        { HandleReply(*this, inSeq, inProps, inNodeId); }
    template<typename T>
    static T& ParserDef(
        T& inParser)
    {
        return MetaVrRequest::ParserDef(inParser)
        .Def("P", &MetaVrDoViewChange::mPimaryNodeId,  seq_t(-1))
        ;
    }
    template<typename T>
    static T& LogIoDef(
        T& inParser)
    {
        return MetaVrRequest::LogIoDef(inParser)
        .Def("P", &MetaVrDoViewChange::mPimaryNodeId,  seq_t(-1))
        ;
    }
protected:
    virtual ~MetaVrDoViewChange()
        {}
};

class MetaVrStartView : public MetaVrRequest
{
public:
    MetaVrStartView()
        : MetaVrRequest(META_VR_START_VIEW, kLogIfOk)
        {}
    virtual ostream& ShowSelf(
        ostream& inOs) const
    {
        return (inOs <<
            "vr-start-view" <<
            " node: "      << mNodeId <<
            " epoch: "     << mEpochSeq <<
            " view: "      << mViewSeq <<
            " committed: " << mCommittedSeq
        );
    }
    virtual void HandleResponse(
        seq_t             inSeq,
        const Properties& inProps,
        NodeId            inNodeId)
        { HandleReply(*this, inSeq, inProps, inNodeId); }
protected:
    virtual ~MetaVrStartView()
        {}
};

class MetaVrReconfiguration : public MetaIdempotentRequest
{
public:
    enum
    {
        kOpTypeNone            = 0,
        kOpTypeAddNode         = 1,
        kOpTypeRemoveNodes     = 2,
        kOpTypeActivateNodes   = 3,
        kOpTypeInactivateNodes = 4,
        kOpTypeSetPrimaryOrder = 5,
        kOpTypeSetTimeouts     = 6,
        kOpTypesCount
    };
    typedef MetaVrSM::Config Config;
    typedef Config::NodeId   NodeId;
    typedef Config::Flags    Flags;

    int             mOpType;
    int             mListSize;
    int             mPrimaryOrder;
    Flags           mNodeFlags;
    NodeId          mNodeId;
    StringBufT<256> mListStr;

    MetaVrReconfiguration()
        : MetaIdempotentRequest(META_VR_RECONFIGURATION, kLogIfOk),
          mOpType(kOpTypeNone),
          mListSize(0),
          mPrimaryOrder(0),
          mNodeFlags(Config::kFlagsNone),
          mNodeId(-1),
          mListStr()
        {}
    virtual ostream& ShowSelf(
        ostream& inOs) const
    {
        return (inOs <<
            "vr-reconfiguration:"
            " type: "  << mOpType <<
            " flags: " << mNodeFlags <<
            " node: "  << mNodeId <<
            " list:"
            " size: "  << mListSize <<
            " "        << mListStr
        );
    }
    virtual bool start();
    bool Validate()
    {
        return ((kOpTypeAddNode == mOpType ? 0 <= mNodeId : 0 < mListSize) &&
            kOpTypeNone < mOpType && mOpType < kOpTypesCount);
    }
    virtual void handle();
    virtual void response(
        ReqOstream& inStream);
    template<typename T>
    static T& ParserDef(T& parser)
    {
        return MetaIdempotentRequest::ParserDef(parser)
        .Def2("Op-type",   "T", &MetaVrReconfiguration::mOpType,
                int(kOpTypeNone))
        .Def2("List-size", "S", &MetaVrReconfiguration::mListSize, 0)
        .Def2("Flags",     "F", &MetaVrReconfiguration::mNodeFlags,
                Flags(Config::kFlagsNone))
        .Def2("Prim-ord",  "O", &MetaVrReconfiguration::mPrimaryOrder, 0)
        .Def2("Node-id",   "N", &MetaVrReconfiguration::mNodeId,
                NodeId(-1))
        .Def2("List",      "L", &MetaVrReconfiguration::mListStr)
        ;
    }
    template<typename T>
    static T& IoParserDef(T& parser)
    {
        // Keep everything except list for debugging.
        return MetaIdempotentRequest::IoParserDef(parser)
        .Def("T", &MetaVrReconfiguration::mOpType,             int(kOpTypeNone))
        .Def("S", &MetaVrReconfiguration::mListSize,                          0)
        .Def("F", &MetaVrReconfiguration::mNodeFlags, Flags(Config::kFlagsNone))
        .Def("O", &MetaVrReconfiguration::mPrimaryOrder,                      0)
        .Def("N", &MetaVrReconfiguration::mNodeId,                   NodeId(-1))
        ;
    }
    template<typename T>
    static T& LogIoDef(T& parser)
    {
        return MetaIdempotentRequest::LogIoDef(parser)
        .Def("T", &MetaVrReconfiguration::mOpType,             int(kOpTypeNone))
        .Def("S", &MetaVrReconfiguration::mListSize,                          0)
        .Def("F", &MetaVrReconfiguration::mNodeFlags, Flags(Config::kFlagsNone))
        .Def("O", &MetaVrReconfiguration::mPrimaryOrder,                      0)
        .Def("N", &MetaVrReconfiguration::mNodeId,                   NodeId(-1))
        .Def("L", &MetaVrReconfiguration::mListStr)
        ;
    }
protected:
    virtual ~MetaVrReconfiguration()
        {}
};

class MetaVrStartEpoch : public MetaVrRequest
{
public:
    MetaVrStartEpoch()
        : MetaVrRequest(META_VR_START_EPOCH, kLogIfOk)
        {}
    virtual ostream& ShowSelf(
        ostream& inOs) const
    {
        return (inOs <<
            "vr-start-epoch" <<
            " node: "      << mNodeId <<
            " epoch: "     << mEpochSeq <<
            " view: "      << mViewSeq <<
            " committed: " << mCommittedSeq
        );
    }
    virtual void HandleResponse(
        seq_t             inSeq,
        const Properties& inProps,
        NodeId            inNodeId)
        { HandleReply(*this, inSeq, inProps, inNodeId); }
protected:
    virtual ~MetaVrStartEpoch()
        {}
};

} // namespace KFS

#endif /* META_VROPS_H */
